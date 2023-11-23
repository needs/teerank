import { prisma } from "./prisma";
import { differenceInSeconds } from "date-fns";
import { removeDuplicatedClients } from "./utils";

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
      },
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

  console.log(`Play timing ${gameServers.length} game servers and ${gameServers.reduce((sum, gameServer) => sum + gameServer.snapshots.length, 0)} snapshots`);

  for (const [index, gameServer] of gameServers.entries()) {
    const snapshots = gameServer.snapshots.sort((a, b) => a.createdAt.getTime() - b.createdAt.getTime());

    console.log(`(${index + 1} / ${gameServers.length}) Play timing ${snapshots.length} snapshots`)

    for (let index = 0; index < snapshots.length - 1; index++) {
      const snapshotStart = snapshots[index];
      const snapshotEnd = snapshots[index + 1];

      const deltaSecond = differenceInSeconds(snapshotEnd.createdAt, snapshotStart.createdAt);
      const deltaPlayTime = deltaSecond > 10 * 60 ? 5 * 60 : deltaSecond;
      const clients = removeDuplicatedClients(snapshotStart.clients);

      await Promise.all(clients.map((client) => {
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
            playerName_gameTypeName: {
              gameTypeName: snapshotStart.map.gameTypeName,
              playerName: client.playerName,
            },
          },
          create: {
            gameTypeName: snapshotStart.map.gameTypeName,
            playerName: client.playerName,
            playTime: deltaPlayTime,
          },
          update: {
            playTime: {
              increment: deltaPlayTime,
            },
          },
        }), prisma.player.update({
          where: {
            name: client.playerName,
          },
          data: {
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

  console.log(`Play timed ${gameServers.length} game servers`);
}

export async function resetPlayTime() {
  await prisma.playerInfo.deleteMany({});
  await prisma.gameServerSnapshot.updateMany({
    where: {
      playTimedAt: {
        not: null,
      },
    },
    data: {
      playTimedAt: null,
    },
  });
  await prisma.gameServerSnapshot.updateMany({
    where: {
      rankedAt: {
        not: null,
      },
    },
    data: {
      rankedAt: null,
    },
  });
  await prisma.player.updateMany({
    where: {
      playTime: {
        not: 0,
      },
    },
    data: {
      playTime: 0,
    },
  });
}
