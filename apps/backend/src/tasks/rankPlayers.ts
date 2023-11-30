import { Prisma, TaskRunStatus } from "@prisma/client";
import { prisma } from "../prisma";
import { removeDuplicatedClients } from "../utils";

type GameServerSnapshotWithClients = Prisma.GameServerSnapshotGetPayload<{
  include: {
    clients: true;
    map: true;
  }
}>;

type RankablePlayer = {
  playerName: string;
  scoreDiff: number;
  rating: number;
  ratingId: number;
}

type RankablePlayers = RankablePlayer[];

type MapOrGameType = {
  type: 'map';
  mapId: number;
} | {
  type: 'gameType';
  gameTypeName: string;
}

async function getPlayerInfo(playerName: string, mapOrGameType: MapOrGameType) {
  switch (mapOrGameType.type) {
    case "map":
      return await prisma.playerInfo.upsert({
        where: {
          playerName_mapId: {
            mapId: mapOrGameType.mapId,
            playerName: playerName,
          },
        },
        create: {
          mapId: mapOrGameType.mapId,
          playerName: playerName,
          rating: 0,
        },
        update: {},
      });
    case "gameType":
      return await prisma.playerInfo.upsert({
        where: {
          playerName_gameTypeName: {
            gameTypeName: mapOrGameType.gameTypeName,
            playerName: playerName,
          },
        },
        create: {
          gameTypeName: mapOrGameType.gameTypeName,
          playerName: playerName,
          rating: 0,
        },
        update: {},
      });
  }
}

async function getRankablePlayers(snapshotStart: GameServerSnapshotWithClients, snapshotEnd: GameServerSnapshotWithClients, useGameTypeRatings: boolean): Promise<RankablePlayers> {
  // Ranking on a different map or gametype doesn't make sense.
  if (snapshotStart.mapId !== snapshotEnd.mapId) {
    return undefined;
  }

  // More than 30 minutes between snapshots increase the oods to rank a different game.
  const elapsedTime = snapshotEnd.createdAt.getTime() - snapshotStart.createdAt.getTime();
  if (elapsedTime > 30 * 60 * 1000) {
    return undefined;
  }

  // Get common players from both snapshots
  const rankablePlayers: RankablePlayers = [];

  const clients = removeDuplicatedClients(snapshotStart.clients);

  for (const clientStart of clients) {
    const clientEnd = snapshotEnd.clients.find((clientEnd) =>
      clientEnd.playerName === clientStart.playerName
    )

    if (clientEnd !== undefined && clientStart.inGame && clientEnd.inGame) {
      const playerInfo = await getPlayerInfo(clientStart.playerName, {
        type: useGameTypeRatings ? 'gameType' : 'map',
        mapId: snapshotStart.mapId,
        gameTypeName: snapshotStart.map.gameTypeName,
      });

      rankablePlayers.push({
        playerName: clientStart.playerName,
        scoreDiff: clientEnd.score - clientStart.score,
        rating: playerInfo.rating,
        ratingId: playerInfo.id
      });
    }
  }

  // At least two players are needed to rank.
  if (rankablePlayers.length < 2) {
    return undefined;
  }

  // If average score is less than -1, then it's probably a new game.
  const scoreAverage = rankablePlayers.reduce((sum, rankablePlayer) => sum + rankablePlayer.scoreDiff, 0) / rankablePlayers.length;
  if (scoreAverage < -1) {
    return undefined;
  }

  return rankablePlayers;
}

// Classic Elo formula for two players
function computeEloDelta(rankablePlayerA: RankablePlayer, rankablePlayerB: RankablePlayer) {
  const K = 25;

  // p() func as defined by Elo.
  function p(delta: number) {
    const clampedDelta = Math.max(-400, Math.min(400, delta));
    return 1.0 / (1.0 + Math.pow(10.0, -clampedDelta / 400.0));
  }

  let W = 0.5;

  if (rankablePlayerA.scoreDiff < rankablePlayerB.scoreDiff) {
    W = 0;
  } else if (rankablePlayerA.scoreDiff > rankablePlayerB.scoreDiff) {
    W = 1;
  }

  return K * (W - p(rankablePlayerA.rating - rankablePlayerB.rating));
}

async function updateRatings(rankablePlayers: RankablePlayers) {
  if (rankablePlayers !== undefined) {
    for (const rankablePlayer of rankablePlayers) {
      const deltas = rankablePlayers.reduce((sum, otherRankablePlayer) => {
        if (rankablePlayer.playerName === otherRankablePlayer.playerName) {
          return sum;
        } else {
          return sum + computeEloDelta(rankablePlayer, otherRankablePlayer)
        }
      }, 0);

      const deltaAverage = deltas / (rankablePlayers.length - 1);

      await prisma.playerInfo.update({
        where: {
          id: rankablePlayer.ratingId,
        },
        data: {
          rating: {
            increment: deltaAverage,
          }
        }
      });
    }
  }
}

export const rankPlayers = async () => {
  // 1. Get all unranked and rankable snapshots grouped by map with their players
  // 2. When there is more than one snapshot, rank the players for each snapshot
  // 3. Mark all snapshots as ranked except the last one

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
    include: {
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
        include: {
          clients: true,
          map: true,
        },
      }
    },
  });

  console.log(`Ranking ${gameServers.length} game servers`);
  console.log(`Ranking ${gameServers.reduce((sum, gameServer) => sum + gameServer.snapshots.length, 0)} snapshots`);

  for (const gameServer of gameServers) {
    const snapshots = gameServer.snapshots.sort((a, b) => a.createdAt.getTime() - b.createdAt.getTime());

    for (let index = 0; index < snapshots.length - 1; index++) {
      const snapshotStart = snapshots[index];
      const snapshotEnd = snapshots[index + 1];

      const rankablePlayersMap = await getRankablePlayers(snapshotStart, snapshotEnd, false);
      await updateRatings(rankablePlayersMap);

      const rankablePlayersGameType = await getRankablePlayers(snapshotStart, snapshotEnd, true);
      await updateRatings(rankablePlayersGameType);

      await prisma.gameServerSnapshot.update({
        where: {
          id: snapshotStart.id,
        },
        data: {
          rankedAt: new Date(),
        }
      });
    }
  }

  return TaskRunStatus.INCOMPLETE;
}
