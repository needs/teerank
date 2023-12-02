import { Prisma, RankMethod, TaskRunStatus } from "@prisma/client";
import { prisma } from "../prisma";
import { fromBase64, toBase64 } from "../utils";
import { differenceInMinutes } from "date-fns";
import groupBy from "lodash.groupby";

type GameServerSnapshotWithClients = Prisma.GameServerSnapshotGetPayload<{
  select: {
    mapId: true;
    createdAt: true;
    clients: {
      select: {
        playerName: true;
        score: true;
        inGame: true;
      }
    };
  }
}>;

type ScoreDeltas = Map<string, number>;

function getScoreDeltas(snapshotStart: GameServerSnapshotWithClients, snapshotEnd: GameServerSnapshotWithClients): ScoreDeltas | undefined {
  // Ranking on a different map or gametype doesn't make sense.
  if (snapshotStart.mapId !== snapshotEnd.mapId) {
    return undefined;
  }

  // More than 30 minutes between snapshots increases odds to rank a different game.
  if (differenceInMinutes(snapshotEnd.createdAt, snapshotStart.createdAt) > 30) {
    return undefined;
  }

  // Get common players from both snapshots
  const scoreDeltas = new Map<string, number>();

  for (const clientStart of snapshotStart.clients) {
    const clientEnd = snapshotEnd.clients.find((clientEnd) =>
      clientEnd.playerName === clientStart.playerName
    )

    if (clientEnd !== undefined && clientStart.inGame && clientEnd.inGame) {
      scoreDeltas.set(clientStart.playerName, clientEnd.score - clientStart.score);
    }
  }

  // At least two players are needed to rank.
  if (scoreDeltas.size < 2) {
    return undefined;
  }

  // If average score is less than -1, then it's probably a new game.
  const scoreAverage = [...scoreDeltas.values()].reduce((sum, scoreDelta) => sum + scoreDelta, 0) / scoreDeltas.size;
  if (scoreAverage < -1) {
    return undefined;
  }

  return scoreDeltas;
}

// Classic Elo formula for two players
function computeEloDelta(scoreA: number, eloA: number, scoreB: number, eloB: number) {
  const K = 25;

  // p() func as defined by Elo.
  function p(delta: number) {
    const clampedDelta = Math.max(-400, Math.min(400, delta));
    return 1.0 / (1.0 + Math.pow(10.0, -clampedDelta / 400.0));
  }

  let W = 0.5;

  if (scoreA < scoreB) {
    W = 0;
  } else if (scoreA > scoreB) {
    W = 1;
  }

  return K * (W - p(eloA - eloB));
}

function updateRatings(
  scoreDeltas: ScoreDeltas,
  getRating: (playerName: string) => number | undefined,
  setRating: (playerName: string, rating: number) => void
) {
  for (const [playerName, scoreDelta] of scoreDeltas.entries()) {
    let eloSum = 0;

    for (const [otherPlayerName, otherScoreDelta] of scoreDeltas.entries()) {
      if (playerName !== otherPlayerName) {
        eloSum += computeEloDelta(
          scoreDelta,
          getRating(playerName) ?? 0,
          otherScoreDelta,
          getRating(otherPlayerName) ?? 0
        );
      }
    }

    const eloAverage = eloSum / (scoreDeltas.size - 1);
    setRating(playerName, (getRating(playerName) ?? 0) + eloAverage);
  }
}

export const rankPlayers = async () => {
  if (process.env.NODE_ENV !== 'test') {
    console.time(`Loading snapshots`);
  }

  const gameServers = await prisma.gameServer.findMany({
    where: {
      snapshots: {
        some: {
          rankedAt: null,
          AND: {
            map: {
              gameType: {
                rankMethod: {
                  not: null,
                },
              },
            },
          }
        }
      }
    },
    select: {
      snapshots: {
        where: {
          rankedAt: null,
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
        orderBy: {
          createdAt: 'asc',
        },
        take: 10,
      }
    },
  });

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Loading snapshots`);
    console.time(`Loading player infos by game type`);
  }

  const playerInfosByGameType = await prisma.$transaction(
    gameServers.flatMap((gameServer) =>
      gameServer.snapshots.flatMap((snapshot) =>
        snapshot.clients.map((client) =>
          prisma.playerInfo.upsert({
            where: {
              playerName_gameTypeName: {
                gameTypeName: snapshot.map.gameTypeName,
                playerName: client.playerName,
              },
            },
            select: {
              rating: true,
              playerName: true,
              gameTypeName: true,
            },
            update: {},
            create: {
              gameTypeName: snapshot.map.gameTypeName,
              playerName: client.playerName,
            },
          })
        )
      )
    ),
  );

  const ratingByGameType = new Map<string, number>();

  const gameTypeKey = (gameTypeName: string, playerName: string) => `${toBase64(gameTypeName)}:${toBase64(playerName)}`;
  const getGameTypeRating = (gameTypeName: string, playerName: string) => ratingByGameType.get(gameTypeKey(gameTypeName, playerName));
  const setGameTypeRating = (gameTypeName: string, playerName: string, rating: number) => ratingByGameType.set(gameTypeKey(gameTypeName, playerName), rating);

  for (const playerInfo of playerInfosByGameType) {
    if (playerInfo.gameTypeName !== null && playerInfo.rating !== null) {
      setGameTypeRating(playerInfo.gameTypeName, playerInfo.playerName, playerInfo.rating);
    }
  }

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Loading player infos by game type`);
    console.time(`Loading player infos by map`);
  }

  const playerInfosByMap = await prisma.$transaction(
    gameServers.flatMap((gameServer) =>
      gameServer.snapshots.flatMap((snapshot) =>
        snapshot.clients.map((client) =>
          prisma.playerInfo.upsert({
            where: {
              playerName_mapId: {
                mapId: snapshot.mapId,
                playerName: client.playerName,
              },
            },
            select: {
              rating: true,
              playerName: true,
              mapId: true,
            },
            update: {},
            create: {
              mapId: snapshot.mapId,
              playerName: client.playerName,
            },
          })
        )
      )
    ),
  );

  const ratingByMap = new Map<string, number>();

  const mapKey = (mapId: number, playerName: string) => `${mapId}:${toBase64(playerName)}`;
  const getMapRating = (mapId: number, playerName: string) => ratingByMap.get(mapKey(mapId, playerName));
  const setMapRating = (mapId: number, playerName: string, rating: number) => ratingByMap.set(mapKey(mapId, playerName), rating);

  for (const playerInfo of playerInfosByMap) {
    if (playerInfo.mapId !== null && playerInfo.rating !== null) {
      setMapRating(playerInfo.mapId, playerInfo.playerName, playerInfo.rating);
    }
  }

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Loading player infos by map`);
    console.time(`Ranking snapshots`);
  }

  const rankedSnapshotIds: number[] = [];

  for (const gameServer of gameServers) {
    const snapshotsPerRankMethod = groupBy(gameServer.snapshots, (snapshot) => snapshot.map.gameType.rankMethod);

    for (const [rankMethod, snapshots] of Object.entries(snapshotsPerRankMethod)) {
      if (rankMethod === RankMethod.ELO) {
        for (let index = 0; index < snapshots.length - 1; index++) {
          const snapshotStart = snapshots[index];
          const snapshotEnd = snapshots[index + 1];

          const scoreDeltas = getScoreDeltas(snapshotStart, snapshotEnd);

          if (scoreDeltas !== undefined) {
            const newMapRatings = new Map<string, number>();

            updateRatings(
              scoreDeltas,
              (playerName) => getMapRating(snapshotStart.mapId, playerName),
              (playerName, rating) => newMapRatings.set(mapKey(snapshotStart.mapId, playerName), rating)
            );

            for (const [key, rating] of newMapRatings.entries()) {
              ratingByMap.set(key, rating);
            }

            const newGameTypeRatings = new Map<string, number>();

            updateRatings(
              scoreDeltas,
              (playerName) => getGameTypeRating(snapshotStart.map.gameTypeName, playerName),
              (playerName, rating) => newGameTypeRatings.set(gameTypeKey(snapshotStart.map.gameTypeName, playerName), rating)
            );

            for (const [key, rating] of newGameTypeRatings.entries()) {
              ratingByGameType.set(key, rating);
            }
          }

          rankedSnapshotIds.push(snapshotStart.id);
        }
      } else if (rankMethod === RankMethod.TIME) {
        for (const snapshot of snapshots) {
          for (const client of snapshot.clients) {
            const newTime = (!client.inGame || Math.abs(client.score) === 9999) ? undefined : -Math.abs(client.score);
            const currentTime = getMapRating(snapshot.mapId, client.playerName);

            if (newTime !== undefined && (currentTime === undefined || newTime > currentTime)) {
              setMapRating(snapshot.mapId, client.playerName, newTime);
            }
          }

          rankedSnapshotIds.push(snapshot.id);
        }
      }
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
          in: rankedSnapshotIds,
        },
      },
      data: {
        rankedAt: new Date(),
      }
    })
  ]);

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Saving ratings`);
  }

  return TaskRunStatus.INCOMPLETE;
}
