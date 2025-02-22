import { prisma } from "../prisma";
import { differenceInSeconds, minutesToSeconds } from "date-fns";
import { removeDuplicatedClients } from "../utils";
import { Worker } from "bullmq";
import { bullmqConnection, QUEUE_NAME_UPDATE_PLAY_TIME } from "@teerank/teerank";
import { getEnvInt } from "@teerank/teerank";

const UPDATE_PLAY_TIME_CONCURRENCY = getEnvInt('UPDATE_PLAY_TIME_CONCURRENCY', 20);

export async function updatePlayTime(snapshotId: number) {
  const snapshot = await prisma.gameServerSnapshot.findUniqueOrThrow({
    where: {
      id: snapshotId,
    },
    select: {
      id: true,
      createdAt: true,
      gameServerId: true,
      mapId: true,
      map: {
        select: {
          gameTypeName: true,
        },
      },
      clients: {
        select: {
          playerName: true,
          clanName: true,
        },
      },
    },
  });

  const snapshotBefore = await prisma.gameServerSnapshot.findFirst({
    select: {
      createdAt: true,
    },
    where: {
      createdAt: {
        lt: snapshot.createdAt,
      },
      gameServerId: snapshot.gameServerId,
    },
    orderBy: {
      createdAt: 'desc',
    },
  });

  const deltaSecond = snapshotBefore === null ? 0 : differenceInSeconds(snapshot.createdAt, snapshotBefore.createdAt);
  const deltaPlayTime = deltaSecond > 10 * 60 ? 5 * 60 : deltaSecond;
  const clients = removeDuplicatedClients(snapshot.clients);

  // Create maps to store accumulated play times
  const playerMapPlayTimes = new Map<string, number>();
  const playerGameTypePlayTimes = new Map<string, number>();
  const playerPlayTimes = new Map<string, number>();
  const clanMapPlayTimes = new Map<string, number>();
  const clanGameTypePlayTimes = new Map<string, number>();
  const clanPlayTimes = new Map<string, number>();
  const clanPlayerPlayTimes = new Map<string, number>();

  // Process all clients and accumulate play times
  for (const client of clients) {
    // Player-Map play time
    const playerMapKey = `${client.playerName}_${snapshot.mapId}`;
    playerMapPlayTimes.set(playerMapKey, (playerMapPlayTimes.get(playerMapKey) || 0) + deltaPlayTime);

    // Player-GameType play time
    const playerGameTypeKey = `${client.playerName}_${snapshot.map.gameTypeName}`;
    playerGameTypePlayTimes.set(playerGameTypeKey, (playerGameTypePlayTimes.get(playerGameTypeKey) || 0) + deltaPlayTime);

    // Player total play time
    playerPlayTimes.set(client.playerName, (playerPlayTimes.get(client.playerName) || 0) + deltaPlayTime);

    if (client.clanName !== null) {
      // Clan-Map play time
      const clanMapKey = `${client.clanName}_${snapshot.mapId}`;
      clanMapPlayTimes.set(clanMapKey, (clanMapPlayTimes.get(clanMapKey) || 0) + deltaPlayTime);

      // Clan-GameType play time
      const clanGameTypeKey = `${client.clanName}_${snapshot.map.gameTypeName}`;
      clanGameTypePlayTimes.set(clanGameTypeKey, (clanGameTypePlayTimes.get(clanGameTypeKey) || 0) + deltaPlayTime);

      // Clan total play time
      clanPlayTimes.set(client.clanName, (clanPlayTimes.get(client.clanName) || 0) + deltaPlayTime);

      // Clan-Player play time
      const clanPlayerKey = `${client.clanName}_${client.playerName}`;
      clanPlayerPlayTimes.set(clanPlayerKey, (clanPlayerPlayTimes.get(clanPlayerKey) || 0) + deltaPlayTime);
    }
  }

  // Batch update database sequentially
  // Update PlayerInfoMap records
  for (const [key, playTime] of playerMapPlayTimes.entries()) {
    const [playerName, mapId] = key.split('_');
    await prisma.playerInfoMap.upsert({
      where: {
        playerName_mapId: {
          mapId: parseInt(mapId),
          playerName,
        },
      },
      update: {
        playTime: { increment: playTime },
      },
      create: {
        player: { connect: { name: playerName } },
        map: { connect: { id: parseInt(mapId) } },
        playTime,
      },
    });
  }

  // Update PlayerInfoGameType records
  for (const [key, playTime] of playerGameTypePlayTimes.entries()) {
    const [playerName, gameTypeName] = key.split('_');
    await prisma.playerInfoGameType.upsert({
      where: {
        playerName_gameTypeName: {
          gameTypeName,
          playerName,
        },
      },
      update: {
        playTime: { increment: playTime },
      },
      create: {
        player: { connect: { name: playerName } },
        gameType: { connect: { name: gameTypeName } },
        playTime,
      },
    });
  }

  // Update Player records
  for (const [playerName, playTime] of playerPlayTimes.entries()) {
    await prisma.player.update({
      where: { name: playerName },
      data: { playTime: { increment: playTime } },
    });
  }

  // Update ClanInfoMap records
  for (const [key, playTime] of clanMapPlayTimes.entries()) {
    const [clanName, mapId] = key.split('_');
    await prisma.clanInfoMap.upsert({
      where: {
        clanName_mapId: {
          mapId: parseInt(mapId),
          clanName,
        },
      },
      update: {
        playTime: { increment: playTime },
      },
      create: {
        clan: { connect: { name: clanName } },
        map: { connect: { id: parseInt(mapId) } },
        playTime,
      },
    });
  }

  // Update ClanInfoGameType records
  for (const [key, playTime] of clanGameTypePlayTimes.entries()) {
    const [clanName, gameTypeName] = key.split('_');
    await prisma.clanInfoGameType.upsert({
      where: {
        clanName_gameTypeName: {
          gameTypeName,
          clanName,
        },
      },
      update: {
        playTime: { increment: playTime },
      },
      create: {
        clan: { connect: { name: clanName } },
        gameType: { connect: { name: gameTypeName } },
        playTime,
      },
    });
  }

  // Update Clan records
  for (const [clanName, playTime] of clanPlayTimes.entries()) {
    await prisma.clan.update({
      where: { name: clanName },
      data: { playTime: { increment: playTime } },
    });
  }

  // Update ClanPlayerInfo records
  for (const [key, playTime] of clanPlayerPlayTimes.entries()) {
    const [clanName, playerName] = key.split('_');
    await prisma.clanPlayerInfo.upsert({
      where: {
        clanName_playerName: {
          clanName,
          playerName,
        },
      },
      update: {
        playTime: { increment: playTime },
      },
      create: {
        clan: { connect: { name: clanName } },
        player: { connect: { name: playerName } },
        playTime,
      },
    });
  }

  // Update GameType, Map, and GameServer records
  await prisma.gameType.update({
    where: { name: snapshot.map.gameTypeName },
    data: { playTime: { increment: deltaPlayTime * clients.length } },
  });

  await prisma.map.update({
    where: { id: snapshot.mapId },
    data: { playTime: { increment: deltaPlayTime * clients.length } },
  });

  await prisma.gameServer.update({
    where: { id: snapshot.gameServerId },
    data: { playTime: { increment: deltaPlayTime * clients.length } },
  });
}

export async function startUpdatePlayTimeWorker() {
  return new Worker(QUEUE_NAME_UPDATE_PLAY_TIME, (job) => updatePlayTime(job.data.snapshotId), {
    connection: bullmqConnection,
    concurrency: UPDATE_PLAY_TIME_CONCURRENCY,
    removeOnComplete: {
      age: minutesToSeconds(10),
    },
  });
}
