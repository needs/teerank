import { Prisma } from "@prisma/client";
import { prisma } from "./prisma";

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

async function getGameTypeMap(gameTypeName: string) {
  return await prisma.map.upsert({
    select: {
      id: true,
    },
    create: {
      gameType: {
        connect: {
          name: gameTypeName,
        },
      },
      name: null,
    },
    update: {},
    where: {
      name_gameTypeName: {
        gameTypeName: gameTypeName,
        name: null,
      },
    },
  });
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

  const map = useGameTypeRatings ? await getGameTypeMap(snapshotStart.map.gameTypeName) : snapshotStart.map;

  for (const clientStart of snapshotStart.clients) {
    const clientEnd = snapshotEnd.clients.find((clientEnd) =>
      clientEnd.playerName === clientStart.playerName
    )

    if (clientEnd !== undefined && clientStart.inGame && clientEnd.inGame) {
      const playerInfo = await prisma.playerInfo.upsert({
        where: {
          playerName_mapId: {
            mapId: map.id,
            playerName: clientStart.playerName,
          },
        },
        create: {
          mapId: map.id,
          rating: 0,
          playerName: clientStart.playerName,
        },
        update: {},
      })

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
          rating: rankablePlayer.rating + deltaAverage,
        }
      });
    }
  }
}

export async function rankPlayers() {
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

      const rankablePlayersGameType = await getRankablePlayers(snapshotStart, snapshotEnd, false);
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
}
