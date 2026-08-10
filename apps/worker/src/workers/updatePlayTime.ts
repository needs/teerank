import {
  incrementClanPlayTimes,
  incrementPlayerPlayTimes,
  upsertClanInfoGameTypePlayTimes,
  upsertClanInfoMapPlayTimes,
  upsertClanPlayerInfoPlayTimes,
  upsertPlayerInfoGameTypePlayTimes,
  upsertPlayerInfoMapPlayTimes,
} from "@prisma/client/sql";
import { prisma } from "../prisma";
import { differenceInSeconds } from "date-fns";
import { sortBy } from "lodash";
import { removeDuplicatedClients } from "../utils";
import { processUpdatePlayTimeJobs, UpdatePlayTimeJobData } from "@teerank/teerank";

export async function updatePlayTime(data: UpdatePlayTimeJobData) {
  const snapshot = await prisma.gameServerSnapshot.findUniqueOrThrow({
    where: {
      id: data.snapshotId,
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

  if (deltaPlayTime === 0) {
    return;
  }

  const clients = removeDuplicatedClients(snapshot.clients);

  if (clients.length === 0) {
    return;
  }

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

  // Each group is written as a single multi-row statement, sorted by its
  // conflict key: unordered multi-row upserts deadlock under concurrent
  // workers with overlapping player sets.
  const playerMaps = sortBy([...playerMapPlayTimes.values()], 'playerName');
  await prisma.$queryRawTyped(upsertPlayerInfoMapPlayTimes(
    playerMaps.map((row) => row.playerName),
    snapshot.mapId,
    playerMaps.map((row) => row.playTime),
  ));

  const playerGameTypes = sortBy([...playerGameTypePlayTimes.values()], 'playerName');
  await prisma.$queryRawTyped(upsertPlayerInfoGameTypePlayTimes(
    playerGameTypes.map((row) => row.playerName),
    snapshot.map.gameTypeName,
    playerGameTypes.map((row) => row.playTime),
  ));

  const players = sortBy([...playerPlayTimes.values()], 'playerName');
  await prisma.$queryRawTyped(incrementPlayerPlayTimes(
    players.map((row) => row.playerName),
    players.map((row) => row.playTime),
  ));

  const clanMaps = sortBy([...clanMapPlayTimes.values()], 'clanName');
  if (clanMaps.length > 0) {
    await prisma.$queryRawTyped(upsertClanInfoMapPlayTimes(
      clanMaps.map((row) => row.clanName),
      snapshot.mapId,
      clanMaps.map((row) => row.playTime),
    ));
  }

  const clanGameTypes = sortBy([...clanGameTypePlayTimes.values()], 'clanName');
  if (clanGameTypes.length > 0) {
    await prisma.$queryRawTyped(upsertClanInfoGameTypePlayTimes(
      clanGameTypes.map((row) => row.clanName),
      snapshot.map.gameTypeName,
      clanGameTypes.map((row) => row.playTime),
    ));
  }

  const clans = sortBy([...clanPlayTimes.values()], 'clanName');
  if (clans.length > 0) {
    await prisma.$queryRawTyped(incrementClanPlayTimes(
      clans.map((row) => row.clanName),
      clans.map((row) => row.playTime),
    ));
  }

  const clanPlayers = sortBy([...clanPlayerPlayTimes.values()], ['clanName', 'playerName']);
  if (clanPlayers.length > 0) {
    await prisma.$queryRawTyped(upsertClanPlayerInfoPlayTimes(
      clanPlayers.map((row) => row.clanName),
      clanPlayers.map((row) => row.playerName),
      clanPlayers.map((row) => row.playTime),
    ));
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
  return await processUpdatePlayTimeJobs(updatePlayTime);
}
