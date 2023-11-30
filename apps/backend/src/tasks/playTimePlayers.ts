import { prisma } from "../prisma";
import { differenceInSeconds } from "date-fns";
import { fromBase64, removeDuplicatedClients, toBase64 } from "../utils";
import { TaskRunStatus } from "@prisma/client";
import { Task } from "../task";

export const playTimePlayers: Task = async () => {
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

  // For performances, keep playtime in memory to upsert in bulk.

  const playerPlayTimeMap = new Map<string, number>();
  const playerPlayTimeGameType = new Map<string, number>();
  const playerPlayTime = new Map<string, number>();

  const clanPlayTimeMap = new Map<string, number>();
  const clanPlayTimeGameType = new Map<string, number>();
  const clanPlayTime = new Map<string, number>();

  const playerClan = new Map<string, { date: Date, clanName: string | null }>();

  const snapshotIds = new Array<number>();

  console.log(`Play timing ${gameServers.length} game servers and ${gameServers.reduce((sum, gameServer) => sum + gameServer.snapshots.length, 0)} snapshots`);
  console.time(`Play timed ${gameServers.length} game servers`);

  for (const gameServer of gameServers) {
    const snapshots = gameServer.snapshots.sort((a, b) => a.createdAt.getTime() - b.createdAt.getTime());

    for (let index = 0; index < snapshots.length - 1; index++) {
      const snapshotStart = snapshots[index];
      const snapshotEnd = snapshots[index + 1];

      const deltaSecond = differenceInSeconds(snapshotEnd.createdAt, snapshotStart.createdAt);
      const deltaPlayTime = deltaSecond > 10 * 60 ? 5 * 60 : deltaSecond;
      const clients = removeDuplicatedClients(snapshotStart.clients);

      clients.forEach((client) => {
        const playerMapKey = `${snapshotStart.mapId}:${toBase64(client.playerName)}`;
        playerPlayTimeMap.set(playerMapKey, (playerPlayTimeMap.get(playerMapKey) ?? 0) + deltaPlayTime);

        const playerGameTypeKey = `${toBase64(snapshotStart.map.gameTypeName)}:${toBase64(client.playerName)}`;
        playerPlayTimeGameType.set(playerGameTypeKey, (playerPlayTimeGameType.get(playerGameTypeKey) ?? 0) + deltaPlayTime);

        const playerKey = toBase64(client.playerName);
        playerPlayTime.set(playerKey, (playerPlayTime.get(playerKey) ?? 0) + deltaPlayTime);

        if (client.clanName !== null) {
          const clanMapKey = `${snapshotStart.mapId}:${toBase64(client.clanName)}`;
          clanPlayTimeMap.set(clanMapKey, (clanPlayTimeMap.get(clanMapKey) ?? 0) + deltaPlayTime);

          const clanGameTypeKey = `${toBase64(snapshotStart.map.gameTypeName)}:${toBase64(client.clanName)}`;
          clanPlayTimeGameType.set(clanGameTypeKey, (clanPlayTimeGameType.get(clanGameTypeKey) ?? 0) + deltaPlayTime);

          const clanKey = toBase64(client.clanName);
          clanPlayTime.set(clanKey, (clanPlayTime.get(clanKey) ?? 0) + deltaPlayTime);
        }

        const currentClan = playerClan.get(playerKey);
        if (currentClan === undefined || currentClan.date < snapshotStart.createdAt) {
          playerClan.set(playerKey, {
            date: snapshotStart.createdAt,
            clanName: client.clanName === "" ? null : client.clanName,
          });
        }
      });

      snapshotIds.push(snapshotStart.id);
    }
  }

  // Since a lot of entries can be updated at once, upserting one by one is too slow.
  // Instead, all existing entries are fetched, deleted and reinserted all at once.

  console.time(`Upserting ${playerPlayTimeMap.size} player map play times`);

  await prisma.$transaction(Array.from(playerPlayTimeMap.entries()).map(([key, value]) => {
    const [mapIdEncoded, playerNameEncoded] = key.split(':');
    const mapId = Number(mapIdEncoded);
    const playerName = fromBase64(playerNameEncoded);

    return prisma.playerInfo.upsert({
      where: {
        playerName_mapId: {
          mapId,
          playerName,
        },
      },
      update: {
        playTime: {
          increment: value,
        },
      },
      create: {
        mapId,
        playerName,
        playTime: value,
      },
    });
  }));

  console.timeEnd(`Upserting ${playerPlayTimeMap.size} player map play times`);
  console.time(`Upserting ${playerPlayTimeGameType.size} player game type play times`);

  await prisma.$transaction(Array.from(playerPlayTimeGameType.entries()).map(([key, value]) => {
    const [gameTypeNameEncoded, playerNameEncoded] = key.split(':');
    const gameTypeName = fromBase64(gameTypeNameEncoded);
    const playerName = fromBase64(playerNameEncoded);

    return prisma.playerInfo.upsert({
      where: {
        playerName_gameTypeName: {
          gameTypeName,
          playerName
        },
      },
      update: {
        playTime: {
          increment: value,
        },
      },
      create: {
        gameTypeName,
        playerName,
        playTime: value,
      },
    });
  }));

  console.timeEnd(`Upserting ${playerPlayTimeGameType.size} player game type play times`);
  console.time(`Upserting ${playerPlayTime.size} player play times`);

  await prisma.$transaction(Array.from(playerPlayTime.entries()).map(([key, value]) => {
    const playerName = fromBase64(key);

    return prisma.player.update({
      where: {
        name: playerName,
      },
      data: {
        playTime: {
          increment: value,
        },
      },
    });
  }));

  console.timeEnd(`Upserting ${playerPlayTime.size} player play times`);
  console.time(`Upserting ${clanPlayTimeMap.size} clan map play times`);

  await prisma.$transaction(Array.from(clanPlayTimeMap.entries()).map(([key, value]) => {
    const [mapIdEncoded, clanNameEncoded] = key.split(':');
    const mapId = Number(mapIdEncoded);
    const clanName = fromBase64(clanNameEncoded);

    return prisma.clanInfo.upsert({
      where: {
        clanName_mapId: {
          mapId,
          clanName,
        },
      },
      update: {
        playTime: {
          increment: value,
        },
      },
      create: {
        mapId,
        clanName,
        playTime: value,
      },
    });
  }));

  console.timeEnd(`Upserting ${clanPlayTimeMap.size} clan map play times`);
  console.time(`Upserting ${clanPlayTimeGameType.size} clan game type play times`);

  await prisma.$transaction(Array.from(clanPlayTimeGameType.entries()).map(([key, value]) => {
    const [gameTypeNameEncoded, clanNameEncoded] = key.split(':');
    const gameTypeName = fromBase64(gameTypeNameEncoded);
    const clanName = fromBase64(clanNameEncoded);

    return prisma.clanInfo.upsert({
      where: {
        clanName_gameTypeName: {
          gameTypeName,
          clanName,
        },
      },
      update: {
        playTime: {
          increment: value,
        },
      },
      create: {
        gameTypeName,
        clanName,
        playTime: value,
      },
    });
  }));

  console.timeEnd(`Upserting ${clanPlayTimeGameType.size} clan game type play times`);
  console.time(`Upserting ${clanPlayTime.size} clan play times`);

  await prisma.$transaction(Array.from(clanPlayTime.entries()).map(([key, value]) => {
    const clanName = fromBase64(key);

    return prisma.clan.update({
      where: {
        name: clanName,
      },
      data: {
        playTime: {
          increment: value,
        },
      },
    });
  }));

  console.timeEnd(`Upserting ${clanPlayTime.size} clan play times`);
  console.time(`Updating ${playerClan.size} player clans`);

  await prisma.$transaction(Array.from(playerClan.entries()).map(([key, value]) => {
    const playerName = fromBase64(key);
    const clanName = value.clanName === "" ? undefined : value.clanName;

    return prisma.player.update({
      where: {
        name: playerName,
      },
      data: {
        clan: clanName === null ? {
          disconnect: true
        } : {
          connectOrCreate: {
            where: {
              name: clanName,
            },
            create: {
              name: clanName,
            },
          },
        },
      },
    });
  }));

  console.timeEnd(`Updating ${playerClan.size} player clans`);
  console.time(`Marking ${snapshotIds.length} snapshots as play timed`);

  await prisma.gameServerSnapshot.updateMany({
    where: {
      id: {
        in: snapshotIds,
      }
    },
    data: {
      playTimedAt: new Date(),
    }
  });

  console.timeEnd(`Marking ${snapshotIds.length} snapshots as play timed`);
  console.timeEnd(`Play timed ${gameServers.length} game servers`);

  return TaskRunStatus.INCOMPLETE;
}
