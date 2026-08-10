import { Prisma } from "@prisma/client";
import { prisma } from "../prisma";
import { differenceInSeconds } from "date-fns";
import { removeDuplicatedClients } from "../utils";
import { processUpdatePlayTimeJobs, UpdatePlayTimeJobData } from "@teerank/teerank";

function compareStrings(a: string, b: string) {
  return a < b ? -1 : a > b ? 1 : 0;
}

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

  // A zero delta would write zero-increment rows to seven tables.
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
  // workers with overlapping player sets. createdAt/updatedAt are set
  // explicitly because they're Prisma-managed.
  const playerMaps = [...playerMapPlayTimes.values()].sort((a, b) => compareStrings(a.playerName, b.playerName));
  await prisma.$executeRaw`
    INSERT INTO "PlayerInfoMap" ("playerName", "mapId", "playTime", "createdAt", "updatedAt")
    VALUES ${Prisma.join(playerMaps.map((row) => Prisma.sql`(${row.playerName}, ${row.mapId}, ${row.playTime}, now(), now())`))}
    ON CONFLICT ("playerName", "mapId") DO UPDATE SET
      "playTime" = "PlayerInfoMap"."playTime" + EXCLUDED."playTime",
      "updatedAt" = now()
  `;

  const playerGameTypes = [...playerGameTypePlayTimes.values()].sort((a, b) => compareStrings(a.playerName, b.playerName));
  await prisma.$executeRaw`
    INSERT INTO "PlayerInfoGameType" ("playerName", "gameTypeName", "playTime", "createdAt", "updatedAt")
    VALUES ${Prisma.join(playerGameTypes.map((row) => Prisma.sql`(${row.playerName}, ${row.gameTypeName}, ${row.playTime}, now(), now())`))}
    ON CONFLICT ("playerName", "gameTypeName") DO UPDATE SET
      "playTime" = "PlayerInfoGameType"."playTime" + EXCLUDED."playTime",
      "updatedAt" = now()
  `;

  const players = [...playerPlayTimes.values()].sort((a, b) => compareStrings(a.playerName, b.playerName));
  await prisma.$executeRaw`
    UPDATE "Player" SET
      "playTime" = "Player"."playTime" + v."playTime",
      "updatedAt" = now()
    FROM (VALUES ${Prisma.join(players.map((row) => Prisma.sql`(${row.playerName}, ${row.playTime})`))}) AS v("playerName", "playTime")
    WHERE "Player"."name" = v."playerName"
  `;

  const clanMaps = [...clanMapPlayTimes.values()].sort((a, b) => compareStrings(a.clanName, b.clanName));
  if (clanMaps.length > 0) {
    await prisma.$executeRaw`
      INSERT INTO "ClanInfoMap" ("clanName", "mapId", "playTime", "createdAt", "updatedAt")
      VALUES ${Prisma.join(clanMaps.map((row) => Prisma.sql`(${row.clanName}, ${row.mapId}, ${row.playTime}, now(), now())`))}
      ON CONFLICT ("clanName", "mapId") DO UPDATE SET
        "playTime" = "ClanInfoMap"."playTime" + EXCLUDED."playTime",
        "updatedAt" = now()
    `;
  }

  const clanGameTypes = [...clanGameTypePlayTimes.values()].sort((a, b) => compareStrings(a.clanName, b.clanName));
  if (clanGameTypes.length > 0) {
    await prisma.$executeRaw`
      INSERT INTO "ClanInfoGameType" ("clanName", "gameTypeName", "playTime", "createdAt", "updatedAt")
      VALUES ${Prisma.join(clanGameTypes.map((row) => Prisma.sql`(${row.clanName}, ${row.gameTypeName}, ${row.playTime}, now(), now())`))}
      ON CONFLICT ("clanName", "gameTypeName") DO UPDATE SET
        "playTime" = "ClanInfoGameType"."playTime" + EXCLUDED."playTime",
        "updatedAt" = now()
    `;
  }

  const clans = [...clanPlayTimes.values()].sort((a, b) => compareStrings(a.clanName, b.clanName));
  if (clans.length > 0) {
    await prisma.$executeRaw`
      UPDATE "Clan" SET
        "playTime" = "Clan"."playTime" + v."playTime",
        "updatedAt" = now()
      FROM (VALUES ${Prisma.join(clans.map((row) => Prisma.sql`(${row.clanName}, ${row.playTime})`))}) AS v("clanName", "playTime")
      WHERE "Clan"."name" = v."clanName"
    `;
  }

  const clanPlayers = [...clanPlayerPlayTimes.values()].sort(
    (a, b) => compareStrings(a.clanName, b.clanName) || compareStrings(a.playerName, b.playerName)
  );
  if (clanPlayers.length > 0) {
    await prisma.$executeRaw`
      INSERT INTO "ClanPlayerInfo" ("clanName", "playerName", "playTime", "createdAt", "updatedAt")
      VALUES ${Prisma.join(clanPlayers.map((row) => Prisma.sql`(${row.clanName}, ${row.playerName}, ${row.playTime}, now(), now())`))}
      ON CONFLICT ("clanName", "playerName") DO UPDATE SET
        "playTime" = "ClanPlayerInfo"."playTime" + EXCLUDED."playTime",
        "updatedAt" = now()
    `;
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
