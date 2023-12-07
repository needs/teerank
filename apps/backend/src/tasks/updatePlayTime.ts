import { prisma } from "../prisma";
import { differenceInSeconds } from "date-fns";
import { fromBase64, removeDuplicatedClients, toBase64 } from "../utils";
import { TaskRunStatus } from "@prisma/client";
import { Task } from "../task";

export const updatePlayTimes: Task = async () => {
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
        take: 10,
        orderBy: {
          createdAt: 'asc',
        },
      }
    },
  });

  // For performances, keep play times in memory to upsert in bulk.

  const playerPlayTimeMap = new Map<string, number>();
  const playerPlayTimeGameType = new Map<string, number>();
  const playerPlayTime = new Map<string, number>();

  const clanPlayTimeMap = new Map<string, number>();
  const clanPlayTimeGameType = new Map<string, number>();
  const clanPlayTime = new Map<string, number>();

  const clanPlayerPlayTime = new Map<string, number>();

  const gameTypePlayTime = new Map<string, number>();
  const mapPlayTime = new Map<string, number>();

  const addPlayerPlayTimeMap = (mapId: number, playerName: string, deltaPlayTime: number) => {
    const key = `${mapId}:${toBase64(playerName)}`;
    playerPlayTimeMap.set(key, (playerPlayTimeMap.get(key) ?? 0) + deltaPlayTime);
  }

  const addPlayerPlayTimeGameType = (gameTypeName: string, playerName: string, deltaPlayTime: number) => {
    const key = `${toBase64(gameTypeName)}:${toBase64(playerName)}`;
    playerPlayTimeGameType.set(key, (playerPlayTimeGameType.get(key) ?? 0) + deltaPlayTime);
  }

  const addPlayerPlayTime = (playerName: string, deltaPlayTime: number) => {
    const key = toBase64(playerName);
    playerPlayTime.set(key, (playerPlayTime.get(key) ?? 0) + deltaPlayTime);
  }

  const addClanPlayTimeMap = (mapId: number, clanName: string, deltaPlayTime: number) => {
    const key = `${mapId}:${toBase64(clanName)}`;
    clanPlayTimeMap.set(key, (clanPlayTimeMap.get(key) ?? 0) + deltaPlayTime);
  }

  const addClanPlayTimeGameType = (gameTypeName: string, clanName: string, deltaPlayTime: number) => {
    const key = `${toBase64(gameTypeName)}:${toBase64(clanName)}`;
    clanPlayTimeGameType.set(key, (clanPlayTimeGameType.get(key) ?? 0) + deltaPlayTime);
  }

  const addClanPlayTime = (clanName: string, deltaPlayTime: number) => {
    const key = toBase64(clanName);
    clanPlayTime.set(key, (clanPlayTime.get(key) ?? 0) + deltaPlayTime);
  }

  const addClanPlayerPlayTime = (playerName: string, clanName: string, deltaPlayTime: number) => {
    const key = `${toBase64(playerName)}:${toBase64(clanName)}`;
    clanPlayerPlayTime.set(key, (clanPlayerPlayTime.get(key) ?? 0) + deltaPlayTime);
  }

  const addGameTypePlayTime = (gameTypeName: string, deltaPlayTime: number) => {
    const key = toBase64(gameTypeName);
    gameTypePlayTime.set(key, (gameTypePlayTime.get(key) ?? 0) + deltaPlayTime);
  }

  const addMapPlayTime = (mapId: number, deltaPlayTime: number) => {
    const key = `${mapId}`;
    mapPlayTime.set(key, (mapPlayTime.get(key) ?? 0) + deltaPlayTime);
  }

  const playerClan = new Map<string, { date: Date, clanName: string | null }>();

  const setPlayerClan = (playerName: string, clanName: string | null, date: Date) => {
    const key = toBase64(playerName);
    const currentClan = playerClan.get(key);
    if (currentClan === undefined || currentClan.date < date) {
      playerClan.set(key, {
        date,
        clanName,
      });
    }
  }

  const snapshotIds = new Array<number>();

  if (process.env.NODE_ENV !== 'test') {
    console.log(`Play timing ${gameServers.length} game servers and ${gameServers.reduce((sum, gameServer) => sum + gameServer.snapshots.length, 0)} snapshots`);
  }

  for (const gameServer of gameServers) {
    for (let index = 0; index < gameServer.snapshots.length - 1; index++) {
      const snapshotStart = gameServer.snapshots[index];
      const snapshotEnd = gameServer.snapshots[index + 1];

      const deltaSecond = differenceInSeconds(snapshotEnd.createdAt, snapshotStart.createdAt);
      const deltaPlayTime = deltaSecond > 10 * 60 ? 5 * 60 : deltaSecond;
      const clients = removeDuplicatedClients(snapshotStart.clients);

      for (const client of clients) {
        addPlayerPlayTimeMap(snapshotStart.mapId, client.playerName, deltaPlayTime);
        addPlayerPlayTimeGameType(snapshotStart.map.gameTypeName, client.playerName, deltaPlayTime);
        addPlayerPlayTime(client.playerName, deltaPlayTime);

        if (client.clanName !== null) {
          addClanPlayTimeMap(snapshotStart.mapId, client.clanName, deltaPlayTime);
          addClanPlayTimeGameType(snapshotStart.map.gameTypeName, client.clanName, deltaPlayTime);
          addClanPlayTime(client.clanName, deltaPlayTime);

          addClanPlayerPlayTime(client.playerName, client.clanName, deltaPlayTime);
        }

        setPlayerClan(client.playerName, client.clanName, snapshotStart.createdAt);
      }

      addGameTypePlayTime(snapshotStart.map.gameTypeName, deltaPlayTime * clients.length);
      addMapPlayTime(snapshotStart.mapId, deltaPlayTime * clients.length);

      snapshotIds.push(snapshotStart.id);
    }

    // Still process last snapshot clients so that player info, clan info and
    // player clan are created/updated.
    if (gameServer.snapshots.length > 0) {
      const lastSnapshot = gameServer.snapshots[gameServer.snapshots.length - 1];
      const clients = removeDuplicatedClients(lastSnapshot.clients);

      for (const client of clients) {
        addPlayerPlayTimeMap(lastSnapshot.mapId, client.playerName, 0);
        addPlayerPlayTimeGameType(lastSnapshot.map.gameTypeName, client.playerName, 0);
        addPlayerPlayTime(client.playerName, 0);

        if (client.clanName !== null) {
          addClanPlayTimeMap(lastSnapshot.mapId, client.clanName, 0);
          addClanPlayTimeGameType(lastSnapshot.map.gameTypeName, client.clanName, 0);
          addClanPlayTime(client.clanName, 0);

          addClanPlayerPlayTime(client.playerName, client.clanName, 0);
        }

        // Game type and map play time are not updated for the last snapshot
        // since both are already created.

        setPlayerClan(client.playerName, client.clanName, lastSnapshot.createdAt);
      }
    }
  }

  // Since a lot of entries can be updated at once, upserting one by one is too slow.
  // Instead, all existing entries are fetched, deleted and reinserted all at once.

  if (process.env.NODE_ENV !== 'test') {
    console.time(`Upserting ${playerPlayTimeMap.size} player map play times`);
  }

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

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Upserting ${playerPlayTimeMap.size} player map play times`);
    console.time(`Upserting ${playerPlayTimeGameType.size} player game type play times`);
  }

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

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Upserting ${playerPlayTimeGameType.size} player game type play times`);
    console.time(`Upserting ${playerPlayTime.size} player play times`);
  }

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

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Upserting ${playerPlayTime.size} player play times`);
    console.time(`Upserting ${clanPlayTimeMap.size} clan map play times`);
  }

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

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Upserting ${clanPlayTimeMap.size} clan map play times`);
    console.time(`Upserting ${clanPlayTimeGameType.size} clan game type play times`);
  }

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

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Upserting ${clanPlayTimeGameType.size} clan game type play times`);
    console.time(`Upserting ${clanPlayTime.size} clan play times`);
  }

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

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Upserting ${clanPlayTime.size} clan play times`);
    console.time(`Updating ${playerClan.size} player clans`);
  }

  await prisma.$transaction(Array.from(playerClan.entries()).map(([key, value]) => {
    const playerName = fromBase64(key);
    const clanName = value.clanName === "" ? null : value.clanName;

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

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Updating ${playerClan.size} player clans`);
    console.time(`Updating ${clanPlayerPlayTime.size} clan player play times`);
  }

  await prisma.$transaction(Array.from(clanPlayerPlayTime.entries()).map(([key, value]) => {
    const [playerNameEncoded, clanNameEncoded] = key.split(':');
    const playerName = fromBase64(playerNameEncoded);
    const clanName = fromBase64(clanNameEncoded);

    return prisma.clanPlayerInfo.upsert({
      where: {
        clanName_playerName: {
          clanName,
          playerName,
        },
      },
      update: {
        playTime: {
          increment: value,
        },
      },
      create: {
        clanName,
        playerName,
        playTime: value,
      },
    });
  }));

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Updating ${clanPlayerPlayTime.size} clan player play times`);
    console.time(`Updating ${gameTypePlayTime.size} game type play times`);
  }

  await prisma.$transaction(Array.from(gameTypePlayTime.entries()).map(([key, value]) => {
    const gameTypeName = fromBase64(key);

    return prisma.gameType.update({
      where: {
        name: gameTypeName,
      },
      data: {
        playTime: {
          increment: value,
        },
      },
    });
  }));

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Updating ${gameTypePlayTime.size} game type play times`);
    console.time(`Updating ${mapPlayTime.size} map play times`);
  }

  await prisma.$transaction(Array.from(mapPlayTime.entries()).map(([key, value]) => {
    const mapId = Number(key);

    return prisma.map.update({
      where: {
        id: mapId,
      },
      data: {
        playTime: {
          increment: value,
        },
      },
    });
  }));

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Updating ${mapPlayTime.size} map play times`);
    console.time(`Marking ${snapshotIds.length} snapshots as play timed`);
  }

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

  if (process.env.NODE_ENV !== 'test') {
    console.timeEnd(`Marking ${snapshotIds.length} snapshots as play timed`);
  }

  return TaskRunStatus.INCOMPLETE;
}
