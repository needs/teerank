import { prisma } from "./prisma";
import { differenceInSeconds } from "date-fns";
import { fromBase64, removeDuplicatedClients, toBase64 } from "./utils";

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

  // For performances, keep playtime in memory to upsert only once
  const playTimeMap = new Map<string, number>();
  const playTimeGameType = new Map<string, number>();
  const playTimePlayer = new Map<string, number>();
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
        const mapKey = `${snapshotStart.mapId} ${toBase64(client.playerName)}`;
        playTimeMap.set(mapKey, playTimeMap.get(mapKey) ?? 0 + deltaPlayTime);

        const gameTypeKey = `${toBase64(snapshotStart.map.gameTypeName)} ${toBase64(client.playerName)}`;
        playTimeGameType.set(gameTypeKey, playTimeGameType.get(gameTypeKey) ?? 0 + deltaPlayTime);

        const playerKey = toBase64(client.playerName);
        playTimePlayer.set(playerKey, playTimePlayer.get(playerKey) ?? 0 + deltaPlayTime);
      });

      snapshotIds.push(snapshotStart.id);
    }
  }

  // Since a lot of entries can be updated at once, upserting one by one is too slow.
  // Instead, all existing entries are fetched, deleted and reinserted all at once.

  console.time(`Upserting ${playTimeMap.size} map play times`);

  await prisma.$transaction(Array.from(playTimeMap.entries()).map(([key, value]) => {
    const [mapIdEncoded, playerNameEncoded] = key.split(' ');
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

  console.timeEnd(`Upserting ${playTimeMap.size} map play times`);
  console.time(`Upserting ${playTimeGameType.size} game type play times`);

  await prisma.$transaction(Array.from(playTimeGameType.entries()).map(([key, value]) => {
    const [gameTypeNameEncoded, playerNameEncoded] = key.split(' ');
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

  console.timeEnd(`Upserting ${playTimeGameType.size} game type play times`);
  console.time(`Upserting ${playTimePlayer.size} player play times`);

  await prisma.$transaction(Array.from(playTimePlayer.entries()).map(([key, value]) => {
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

  console.timeEnd(`Upserting ${playTimePlayer.size} player play times`);
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
