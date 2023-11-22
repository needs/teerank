import { prisma } from "./prisma";
import { differenceInSeconds } from "date-fns";

async function getGameTypeMap(gameTypeName: string) {
  return await prisma.map.upsert({
    select: {
      id: true,
    },
    where: {
      name_gameTypeName: {
        gameTypeName: gameTypeName,
        name: null,
      },
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
  });
}

export async function playTimePlayers() {
  // 1. Get all snapshots not yet play timed, grouped by servers
  // 2. When there is more than one snapshot:
  //    - Calculate the time between the snapshots
  //    - If the time is more than 10 minutes, use 5 minutes
  //    - For each client, increase play time
  //    - Mark the snapshot as play timed

  const gameServers = await prisma.gameServer.findMany({
    where: {
      snapshots: {
        some: {
          playTimedAt: null,
        }
      }
    },
    include: {
      snapshots: {
        where: {
          playTimedAt: null,
        },
        include: {
          clients: true,
          map: true,
        },
      }
    },
  });

  console.log(`Play timing ${gameServers.length} game servers`);
  console.log(`Play timing ${gameServers.reduce((sum, gameServer) => sum + gameServer.snapshots.length, 0)} snapshots`);

  for (const gameServer of gameServers) {
    const snapshots = gameServer.snapshots.sort((a, b) => a.createdAt.getTime() - b.createdAt.getTime());

    for (let index = 0; index < snapshots.length - 1; index++) {
      const snapshotStart = snapshots[index];
      const snapshotEnd = snapshots[index + 1];

      const gameTypeMap = await getGameTypeMap(snapshotStart.map.gameTypeName);

      const deltaSecond = differenceInSeconds(snapshotStart.createdAt, snapshotEnd.createdAt);
      const deltaPlayTime = deltaSecond > 10 * 60 ? 5 * 60 : deltaSecond;

      await Promise.all(snapshotStart.clients.map((client) => {
        return Promise.all([prisma.playerInfo.upsert({
          where: {
            playerName_mapId: {
              mapId: snapshotStart.mapId,
              playerName: client.playerName,
            },
          },
          create: {
            mapId: snapshotStart.mapId,
            playerName: client.playerName,
            playTime: deltaPlayTime,
          },
          update: {
            playTime: {
              increment: deltaPlayTime,
            },
          },
        }), prisma.playerInfo.upsert({
          where: {
            playerName_mapId: {
              mapId: gameTypeMap.id,
              playerName: client.playerName,
            },
          },
          create: {
            mapId: snapshotStart.mapId,
            playerName: client.playerName,
            playTime: deltaPlayTime,
          },
          update: {
            playTime: {
              increment: deltaPlayTime,
            },
          },
        })]);
      }));

      await prisma.gameServerSnapshot.update({
        where: {
          id: snapshotStart.id,
        },
        data: {
          playTimedAt: new Date(),
        }
      });
    }
  }
}
