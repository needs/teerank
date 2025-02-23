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

  // Create maps to store accumulated play times with structured values
  type PlayerMapPlayTime = { playerName: string; mapId: number; playTime: number };
  type PlayerGameTypePlayTime = { playerName: string; gameTypeName: string; playTime: number };
  type PlayerPlayTime = { playerName: string; playTime: number };
  type ClanMapPlayTime = { clanName: string; mapId: number; playTime: number };
  type ClanGameTypePlayTime = { clanName: string; gameTypeName: string; playTime: number };
  type ClanPlayTime = { clanName: string; playTime: number };
  type ClanPlayerPlayTime = { clanName: string; playerName: string; playTime: number };

  const playerMapPlayTimes = new Map<string, PlayerMapPlayTime>();
  const playerGameTypePlayTimes = new Map<string, PlayerGameTypePlayTime>();
  const playerPlayTimes = new Map<string, PlayerPlayTime>();
  const clanMapPlayTimes = new Map<string, ClanMapPlayTime>();
  const clanGameTypePlayTimes = new Map<string, ClanGameTypePlayTime>();
  const clanPlayTimes = new Map<string, ClanPlayTime>();
  const clanPlayerPlayTimes = new Map<string, ClanPlayerPlayTime>();

  // Process all clients and accumulate play times
  for (const client of clients) {
    // Player-Map play time
    const playerMapKey = `${client.playerName}_${snapshot.mapId}`;
    const existingPlayerMapTime = playerMapPlayTimes.get(playerMapKey)?.playTime ?? 0;
    playerMapPlayTimes.set(playerMapKey, {
      playerName: client.playerName,
      mapId: snapshot.mapId,
      playTime: existingPlayerMapTime + deltaPlayTime
    });

    // Player-GameType play time
    const playerGameTypeKey = `${client.playerName}_${snapshot.map.gameTypeName}`;
    const existingPlayerGameTypeTime = playerGameTypePlayTimes.get(playerGameTypeKey)?.playTime ?? 0;
    playerGameTypePlayTimes.set(playerGameTypeKey, {
      playerName: client.playerName,
      gameTypeName: snapshot.map.gameTypeName,
      playTime: existingPlayerGameTypeTime + deltaPlayTime
    });

    // Player total play time
    const existingPlayerTime = playerPlayTimes.get(client.playerName)?.playTime ?? 0;
    playerPlayTimes.set(client.playerName, {
      playerName: client.playerName,
      playTime: existingPlayerTime + deltaPlayTime
    });

    if (client.clanName !== null) {
      // Clan-Map play time
      const clanMapKey = `${client.clanName}_${snapshot.mapId}`;
      const existingClanMapTime = clanMapPlayTimes.get(clanMapKey)?.playTime ?? 0;
      clanMapPlayTimes.set(clanMapKey, {
        clanName: client.clanName,
        mapId: snapshot.mapId,
        playTime: existingClanMapTime + deltaPlayTime
      });

      // Clan-GameType play time
      const clanGameTypeKey = `${client.clanName}_${snapshot.map.gameTypeName}`;
      const existingClanGameTypeTime = clanGameTypePlayTimes.get(clanGameTypeKey)?.playTime ?? 0;
      clanGameTypePlayTimes.set(clanGameTypeKey, {
        clanName: client.clanName,
        gameTypeName: snapshot.map.gameTypeName,
        playTime: existingClanGameTypeTime + deltaPlayTime
      });

      // Clan total play time
      const existingClanTime = clanPlayTimes.get(client.clanName)?.playTime ?? 0;
      clanPlayTimes.set(client.clanName, {
        clanName: client.clanName,
        playTime: existingClanTime + deltaPlayTime
      });

      // Clan-Player play time
      const clanPlayerKey = `${client.clanName}_${client.playerName}`;
      const existingClanPlayerTime = clanPlayerPlayTimes.get(clanPlayerKey)?.playTime ?? 0;
      clanPlayerPlayTimes.set(clanPlayerKey, {
        clanName: client.clanName,
        playerName: client.playerName,
        playTime: existingClanPlayerTime + deltaPlayTime
      });
    }
  }

  // Update PlayerInfoMap records
  for (const data of playerMapPlayTimes.values()) {
    await prisma.playerInfoMap.upsert({
      where: {
        playerName_mapId: {
          mapId: data.mapId,
          playerName: data.playerName,
        },
      },
      update: {
        playTime: { increment: data.playTime },
      },
      create: {
        player: { connect: { name: data.playerName } },
        map: { connect: { id: data.mapId } },
        playTime: data.playTime,
      },
    });
  }

  // Update PlayerInfoGameType records
  for (const data of playerGameTypePlayTimes.values()) {
    await prisma.playerInfoGameType.upsert({
      where: {
        playerName_gameTypeName: {
          gameTypeName: data.gameTypeName,
          playerName: data.playerName,
        },
      },
      update: {
        playTime: { increment: data.playTime },
      },
      create: {
        player: { connect: { name: data.playerName } },
        gameType: { connect: { name: data.gameTypeName } },
        playTime: data.playTime,
      },
    });
  }

  // Update Player records
  for (const data of playerPlayTimes.values()) {
    await prisma.player.update({
      where: { name: data.playerName },
      data: { playTime: { increment: data.playTime } },
    });
  }

  // Update ClanInfoMap records
  for (const data of clanMapPlayTimes.values()) {
    await prisma.clanInfoMap.upsert({
      where: {
        clanName_mapId: {
          mapId: data.mapId,
          clanName: data.clanName,
        },
      },
      update: {
        playTime: { increment: data.playTime },
      },
      create: {
        clan: { connect: { name: data.clanName } },
        map: { connect: { id: data.mapId } },
        playTime: data.playTime,
      },
    });
  }

  // Update ClanInfoGameType records
  for (const data of clanGameTypePlayTimes.values()) {
    await prisma.clanInfoGameType.upsert({
      where: {
        clanName_gameTypeName: {
          gameTypeName: data.gameTypeName,
          clanName: data.clanName,
        },
      },
      update: {
        playTime: { increment: data.playTime },
      },
      create: {
        clan: { connect: { name: data.clanName } },
        gameType: { connect: { name: data.gameTypeName } },
        playTime: data.playTime,
      },
    });
  }

  // Update Clan records
  for (const data of clanPlayTimes.values()) {
    await prisma.clan.update({
      where: { name: data.clanName },
      data: { playTime: { increment: data.playTime } },
    });
  }

  // Update ClanPlayerInfo records
  for (const data of clanPlayerPlayTimes.values()) {
    await prisma.clanPlayerInfo.upsert({
      where: {
        clanName_playerName: {
          clanName: data.clanName,
          playerName: data.playerName,
        },
      },
      update: {
        playTime: { increment: data.playTime },
      },
      create: {
        clan: { connect: { name: data.clanName } },
        player: { connect: { name: data.playerName } },
        playTime: data.playTime,
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
