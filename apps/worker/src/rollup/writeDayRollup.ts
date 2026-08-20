import { chunk } from "lodash";
import { minutesToMilliseconds } from "date-fns";
import { formatUtcDay } from "@teerank/teerank";
import { prisma } from "../prisma";
import { DayRollup } from "./aggregateDay";
import { ensureRollupPartitions } from "./partitions";

const LOOKUP_CHUNK_SIZE = 5_000;
const INSERT_CHUNK_SIZE = 2_000;

async function lookupIds(
  names: string[],
  findMany: (names: string[]) => Promise<{ name: string; id: number }[]>
) {
  const ids = new Map<string, number>();

  for (const names_ of chunk(names, LOOKUP_CHUNK_SIZE)) {
    for (const { name, id } of await findMany(names_)) {
      ids.set(name, id);
    }
  }

  return ids;
}

export async function writeDayRollup(day: Date, rollup: DayRollup) {
  await ensureRollupPartitions(day);

  const [playerIds, clanIds, gameTypeIds] = await Promise.all([
    lookupIds(
      rollup.players.map((row) => row.playerName),
      (names) =>
        prisma.player.findMany({
          where: { name: { in: names } },
          select: { name: true, id: true },
        })
    ),
    lookupIds(
      rollup.clans.map((row) => row.clanName),
      (names) =>
        prisma.clan.findMany({
          where: { name: { in: names } },
          select: { name: true, id: true },
        })
    ),
    lookupIds(
      rollup.gameTypes.map((row) => row.gameTypeName),
      (names) =>
        prisma.gameType.findMany({
          where: { name: { in: names } },
          select: { name: true, id: true },
        })
    ),
  ]);

  // Names that no longer resolve (deleted players/clans) are dropped: the
  // rollup has no string columns to keep them under.
  let droppedRows = 0;

  const playerRows = rollup.players.flatMap((row) => {
    const playerId = playerIds.get(row.playerName);

    if (playerId === undefined) {
      droppedRows += 1;
      return [];
    }

    return {
      day,
      playerId,
      playTime: row.playTime,
    };
  });

  const serverRows = rollup.serverDays.map((row) => ({
    day,
    gameServerId: row.gameServerId,
    avgClients: row.avgClients,
    maxClients: row.maxClients,
  }));

  const mapRows = rollup.maps.map((row) => ({
    day,
    mapId: row.mapId,
    playTime: row.playTime,
    playerCount: row.playerCount,
  }));

  const gameTypeRows = rollup.gameTypes.flatMap((row) => {
    const gameTypeId = gameTypeIds.get(row.gameTypeName);

    if (gameTypeId === undefined) {
      droppedRows += 1;
      return [];
    }

    return {
      day,
      gameTypeId,
      playTime: row.playTime,
      playerCount: row.playerCount,
    };
  });

  const clanRows = rollup.clans.flatMap((row) => {
    const clanId = clanIds.get(row.clanName);

    if (clanId === undefined) {
      droppedRows += 1;
      return [];
    }

    return {
      day,
      clanId,
      playTime: row.playTime,
      playerCount: row.playerCount,
    };
  });

  // Inserting a day in key order keeps the btrees ~90% full instead of the
  // ~70% random inserts leave; the reverse indexes lead with the same id.
  playerRows.sort((a, b) => a.playerId - b.playerId);
  serverRows.sort((a, b) => a.gameServerId - b.gameServerId);
  mapRows.sort((a, b) => a.mapId - b.mapId);
  gameTypeRows.sort((a, b) => a.gameTypeId - b.gameTypeId);
  clanRows.sort((a, b) => a.clanId - b.clanId);

  await prisma.$transaction(
    async (tx) => {
      await tx.playerDay.deleteMany({ where: { day } });
      await tx.serverDay.deleteMany({ where: { day } });
      await tx.mapDay.deleteMany({ where: { day } });
      await tx.gameTypeDay.deleteMany({ where: { day } });
      await tx.clanDay.deleteMany({ where: { day } });

      for (const rows of chunk(playerRows, INSERT_CHUNK_SIZE)) {
        await tx.playerDay.createMany({ data: rows });
      }
      for (const rows of chunk(serverRows, INSERT_CHUNK_SIZE)) {
        await tx.serverDay.createMany({ data: rows });
      }
      for (const rows of chunk(mapRows, INSERT_CHUNK_SIZE)) {
        await tx.mapDay.createMany({ data: rows });
      }
      for (const rows of chunk(gameTypeRows, INSERT_CHUNK_SIZE)) {
        await tx.gameTypeDay.createMany({ data: rows });
      }
      for (const rows of chunk(clanRows, INSERT_CHUNK_SIZE)) {
        await tx.clanDay.createMany({ data: rows });
      }
    },
    { timeout: minutesToMilliseconds(5), maxWait: minutesToMilliseconds(1) }
  );

  const dayLabel = formatUtcDay(day);
  console.log(
    `Rolled up ${dayLabel}: ${playerRows.length} PlayerDay, ` +
      `${serverRows.length} ServerDay, ${mapRows.length} MapDay, ` +
      `${gameTypeRows.length} GameTypeDay, ${clanRows.length} ClanDay` +
      (droppedRows > 0 ? ` (${droppedRows} rows dropped: unresolved names)` : '')
  );
}
