import { RankMethod } from "@prisma/client";
import { prisma } from "../prisma";
import { fromBase64, toBase64 } from "../utils";
import { rankPlayersElo } from "../rankMethods/rankPlayersElo";
import { rankPlayersTime } from "../rankMethods/rankPlayersTime";

export type SnapshotToRank = Awaited<ReturnType<typeof getSnapshots>>[number];

export type RankFunctionArgs = {
  snapshot: SnapshotToRank,
  getMapRating: (mapId: number, playerName: string) => number | undefined,
  setMapRating: (mapId: number, playerName: string, rating: number) => void,
  getGameTypeRating: (gameTypeName: string, playerName: string) => number | undefined,
  setGameTypeRating: (gameTypeName: string, playerName: string, rating: number) => void,
};

async function getSnapshots(rangeStart: number, rangeEnd: number) {
  return await prisma.gameServerSnapshot.findMany({
    where: {
      id: {
        gte: rangeStart,
        lte: rangeEnd,
      },
      map: {
        gameType: {
          rankMethod: {
            not: null,
          },
        },
      },
    },
    select: {
      id: true,
      createdAt: true,
      gameServerId: true,
      clients: {
        where: {
          playerName: {
            // Don't rank connecting players because their score is meaningless.
            not: "(connecting)",
          }
        },
        select: {
          playerName: true,
          score: true,
          inGame: true,
        }
      },
      mapId: true,
      map: {
        select: {
          gameTypeName: true,
          gameType: {
            select: {
              rankMethod: true,
            }
          },
        },
      },
    },
  });
}

export async function rankPlayers(rangeStart: number, rangeEnd: number) {
  if (process.env.NODE_ENV !== 'test') {
    console.time(`Loading snapshots`);
  }

  const snapshots = await getSnapshots(rangeStart, rangeEnd);

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Loading snapshots`);
    console.time(`Loading player infos by game type`);
  }

  const ratingByGameType = new Map<string, number | undefined>();

  const gameTypeKey = (gameTypeName: string, playerName: string) => `${toBase64(gameTypeName)}:${toBase64(playerName)}`;
  const getGameTypeRating = (gameTypeName: string, playerName: string) => ratingByGameType.get(gameTypeKey(gameTypeName, playerName));
  const setGameTypeRating = (gameTypeName: string, playerName: string, rating: number | undefined) => ratingByGameType.set(gameTypeKey(gameTypeName, playerName), rating);

  for (const snapshot of snapshots) {
    for (const client of snapshot.clients) {
      setGameTypeRating(snapshot.map.gameTypeName, client.playerName, undefined);
    }
  }

  const playerInfosByGameType = await prisma.$transaction(
    [...ratingByGameType.keys()].map((key) => {
      const [gameTypeNameEncoded, playerNameEncoded] = key.split(':');
      const gameTypeName = fromBase64(gameTypeNameEncoded);
      const playerName = fromBase64(playerNameEncoded);

      return prisma.playerInfo.upsert({
        where: {
          playerName_gameTypeName: {
            gameTypeName,
            playerName,
          },
        },
        select: {
          rating: true,
          playerName: true,
          gameTypeName: true,
        },
        update: {},
        create: {
          gameTypeName,
          playerName,
        },
      });
    })
  );

  for (const playerInfo of playerInfosByGameType) {
    if (playerInfo.gameTypeName !== null) {
      setGameTypeRating(playerInfo.gameTypeName, playerInfo.playerName, playerInfo.rating ?? undefined);
    }
  }

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Loading player infos by game type`);
    console.time(`Loading player infos by map`);
  }

  const ratingByMap = new Map<string, number | undefined>();

  const mapKey = (mapId: number, playerName: string) => `${mapId}:${toBase64(playerName)}`;
  const getMapRating = (mapId: number, playerName: string) => ratingByMap.get(mapKey(mapId, playerName));
  const setMapRating = (mapId: number, playerName: string, rating: number | undefined) => ratingByMap.set(mapKey(mapId, playerName), rating);

  for (const snapshot of snapshots) {
    for (const client of snapshot.clients) {
      setMapRating(snapshot.mapId, client.playerName, undefined);
    }
  }

  const playerInfosByMap = await prisma.$transaction(
    [...ratingByMap.keys()].map((key) => {
      const [mapIdEncoded, playerNameEncoded] = key.split(':');
      const mapId = Number(mapIdEncoded);
      const playerName = fromBase64(playerNameEncoded);

      return prisma.playerInfo.upsert({
        where: {
          playerName_mapId: {
            mapId: mapId,
            playerName: playerName,
          },
        },
        select: {
          rating: true,
          playerName: true,
          mapId: true,
        },
        update: {},
        create: {
          mapId: mapId,
          playerName: playerName,
        },
      })
    })
  );

  for (const playerInfo of playerInfosByMap) {
    if (playerInfo.mapId !== null) {
      setMapRating(playerInfo.mapId, playerInfo.playerName, playerInfo.rating ?? undefined);
    }
  }

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Loading player infos by map`);
    console.time(`Ranking snapshots`);
  }

  for (const snapshot of snapshots) {
    const rankFunctionArgs = {
      snapshot,
      getMapRating,
      setMapRating,
      getGameTypeRating,
      setGameTypeRating,
    }

    switch (snapshot.map.gameType.rankMethod) {
      case RankMethod.ELO:
        await rankPlayersElo(rankFunctionArgs);
        break;
      case RankMethod.TIME:
        await rankPlayersTime(rankFunctionArgs);
        break;
    }
  }

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Ranking snapshots`);
    console.time(`Saving ratings`);
  }

  await prisma.$transaction([
    ...[...ratingByMap.entries()].map(([key, rating]) => {
      const [mapIdEncoded, playerNameEncoded] = key.split(':');
      const mapId = Number(mapIdEncoded);
      const playerName = fromBase64(playerNameEncoded);

      return prisma.playerInfo.update({
        where: {
          playerName_mapId: {
            mapId,
            playerName,
          },
        },
        data: {
          rating,
        },
      });
    }),

    ...[...ratingByGameType.entries()].map(([key, rating]) => {
      const [gameTypeNameEncoded, playerNameEncoded] = key.split(':');
      const gameTypeName = fromBase64(gameTypeNameEncoded);
      const playerName = fromBase64(playerNameEncoded);

      return prisma.playerInfo.update({
        where: {
          playerName_gameTypeName: {
            gameTypeName,
            playerName,
          },
        },
        data: {
          rating,
        },
      });
    }),

    prisma.gameServerSnapshot.updateMany({
      where: {
        id: {
          in: snapshots.map((snapshot) => snapshot.id),
        },
        map: {
          gameType: {
            rankMethod: {
              not: null,
            },
          },
        }
      },
      data: {
        rankedAt: new Date(),
      }
    })
  ]);

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Saving ratings`);
  }
}
