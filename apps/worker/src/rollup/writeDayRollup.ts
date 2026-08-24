import { chunk } from "lodash";
import { minutesToMilliseconds } from "date-fns";
import { formatUtcDay, isStubName } from "@teerank/teerank";
import { incrementPlayerPollCounts, upsertPlayerPartners } from "@prisma/client/sql";
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

  const stubPlayerIds = new Set<number>();

  const [playerIds, clanIds, gameTypeIds] = await Promise.all([
    lookupIds(
      rollup.players.map((row) => row.playerName),
      async (names) => {
        const players = await prisma.player.findMany({
          where: { name: { in: names } },
          select: { name: true, id: true, pollCount: true, occurrenceCount: true },
        });

        for (const player of players) {
          if (isStubName(player.pollCount, player.occurrenceCount)) {
            stubPlayerIds.add(player.id);
          }
        }

        return players;
      }
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

  const playerRows: { day: Date; playerId: number; playTime: number }[] = [];
  const pollCountRows: { playerId: number; pollCount: number; occurrenceCount: number }[] = [];

  for (const row of rollup.players) {
    const playerId = playerIds.get(row.playerName);

    if (playerId === undefined) {
      droppedRows += 1;
      continue;
    }

    playerRows.push({ day, playerId, playTime: row.playTime });
    pollCountRows.push({
      playerId,
      pollCount: row.pollCount,
      occurrenceCount: row.occurrenceCount,
    });
  }

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

  // A stub name is many people, so pairs involving one are meaningless.
  const partnerRows = rollup.partners.flatMap((row) => {
    const playerId = playerIds.get(row.playerName);
    const partnerId = playerIds.get(row.partnerName);

    if (playerId === undefined || partnerId === undefined) {
      droppedRows += 1;
      return [];
    }

    if (stubPlayerIds.has(playerId) || stubPlayerIds.has(partnerId)) {
      return [];
    }

    return [
      { playerId, partnerId, playTime: row.playTime },
      { playerId: partnerId, partnerId: playerId, playTime: row.playTime },
    ];
  });

  // Inserting a day in key order keeps the btrees ~90% full instead of the
  // ~70% random inserts leave; the reverse indexes lead with the same id.
  playerRows.sort((a, b) => a.playerId - b.playerId);
  pollCountRows.sort((a, b) => a.playerId - b.playerId);
  serverRows.sort((a, b) => a.gameServerId - b.gameServerId);
  mapRows.sort((a, b) => a.mapId - b.mapId);
  gameTypeRows.sort((a, b) => a.gameTypeId - b.gameTypeId);
  clanRows.sort((a, b) => a.clanId - b.clanId);
  partnerRows.sort((a, b) => a.playerId - b.playerId || a.partnerId - b.partnerId);

  await prisma.$transaction(
    async (tx) => {
      const alreadyRolledUp =
        (await tx.globalDay.findUnique({ where: { day }, select: { day: true } })) !== null;

      await tx.globalDay.upsert({
        where: { day },
        create: { day, playerCount: playerRows.length },
        update: { playerCount: playerRows.length },
      });

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

      if (!alreadyRolledUp) {
        for (const rows of chunk(partnerRows, INSERT_CHUNK_SIZE)) {
          await tx.$queryRawTyped(
            upsertPlayerPartners(
              rows.map((row) => row.playerId),
              rows.map((row) => row.partnerId),
              rows.map((row) => row.playTime),
              day
            )
          );
        }

        for (const rows of chunk(pollCountRows, INSERT_CHUNK_SIZE)) {
          await tx.$queryRawTyped(
            incrementPlayerPollCounts(
              rows.map((row) => row.playerId),
              rows.map((row) => row.pollCount),
              rows.map((row) => row.occurrenceCount)
            )
          );
        }
      }
    },
    { timeout: minutesToMilliseconds(5), maxWait: minutesToMilliseconds(1) }
  );

  const dayLabel = formatUtcDay(day);
  console.log(
    `Rolled up ${dayLabel}: ${playerRows.length} PlayerDay, ` +
      `${serverRows.length} ServerDay, ${mapRows.length} MapDay, ` +
      `${gameTypeRows.length} GameTypeDay, ${clanRows.length} ClanDay, ` +
      `${partnerRows.length} PlayerPartner` +
      (droppedRows > 0 ? ` (${droppedRows} rows dropped: unresolved names)` : '')
  );
}
