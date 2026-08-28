import { chunk } from "lodash";
import { minutesToMilliseconds } from "date-fns";
import { Prisma } from "@prisma/client";
import {
  createRollupPartition,
  incrementDdnetPlayers,
  incrementMapDayFinishes,
  incrementPlayerDayFinishes,
  insertDdnetRecords,
  listDdnetCurrentRecords,
  setMapDayFinishes,
  setPlayerDayFinishes,
  upsertDdnetPartnerMaps,
  upsertDdnetPartners,
  upsertDdnetPlayerNames,
  upsertDdnetRaceBests,
  upsertDdnetTeamRacePlayers,
  upsertDdnetTeamRaces,
} from "@prisma/client/sql";
import { isStubName } from "@teerank/teerank";
import { prisma } from "../prisma";
import { DdnetRaceRow, DdnetTeamRaceRow } from "./stream";

export const DDNET_GAME_TYPE = 'DDraceNetwork';

const LOOKUP_CHUNK_SIZE = 5_000;
const INSERT_CHUNK_SIZE = 2_000;

export type TransactionClient = Prisma.TransactionClient;

export type RecordState = Map<number, number>;

export type RecordCandidate = {
  mapId: number;
  playerId: number;
  time: number;
  timestamp: string; // ISO
};

export async function loadRecordState(): Promise<RecordState> {
  const rows = await prisma.$queryRawTyped(listDdnetCurrentRecords());
  return new Map(rows.map((row) => [row.mapId, row.time]));
}

export async function ensureFinishPartitions(ym: string) {
  const from = new Date(`${ym}-01T00:00:00Z`);
  const to = new Date(Date.UTC(from.getUTCFullYear(), from.getUTCMonth() + 1, 1));

  for (const table of ['PlayerDay', 'MapDay']) {
    await prisma.$queryRawTyped(createRollupPartition(table, from, to));
  }
}

function utcDay(date: Date) {
  return new Date(Date.UTC(date.getUTCFullYear(), date.getUTCMonth(), date.getUTCDate()));
}

export async function resolveMapIds(mapNames: string[]) {
  const distinct = [...new Set(mapNames)];
  const ids = new Map<string, number>();

  for (const names of chunk(distinct, LOOKUP_CHUNK_SIZE)) {
    const found = await prisma.map.findMany({
      where: { gameTypeName: DDNET_GAME_TYPE, name: { in: names } },
      select: { name: true, id: true },
    });
    for (const map of found) {
      ids.set(map.name, map.id);
    }

    const missing = names.filter((name) => !ids.has(name));
    if (missing.length > 0) {
      await prisma.gameType.upsert({
        where: { name: DDNET_GAME_TYPE },
        create: { name: DDNET_GAME_TYPE, rankMethod: 'TIME' },
        update: {},
      });
      await prisma.map.createMany({
        data: missing.map((name) => ({ name, gameTypeName: DDNET_GAME_TYPE })),
        skipDuplicates: true,
      });
      const created = await prisma.map.findMany({
        where: { gameTypeName: DDNET_GAME_TYPE, name: { in: missing } },
        select: { name: true, id: true },
      });
      for (const map of created) {
        ids.set(map.name, map.id);
      }
    }
  }

  return ids;
}

async function resolvePlayerIds(lastSeenByName: Map<string, Date>) {
  const names = [...lastSeenByName.keys()];

  for (const names_ of chunk(names, INSERT_CHUNK_SIZE)) {
    await prisma.$queryRawTyped(
      upsertDdnetPlayerNames(names_, names_.map((name) => lastSeenByName.get(name)!))
    );
  }

  const ids = new Map<string, number>();
  const stubIds = new Set<number>();

  for (const names_ of chunk(names, LOOKUP_CHUNK_SIZE)) {
    const players = await prisma.player.findMany({
      where: { name: { in: names_ } },
      select: { name: true, id: true, pollCount: true, occurrenceCount: true },
    });
    for (const player of players) {
      ids.set(player.name, player.id);
      if (isStubName(player.pollCount, player.occurrenceCount)) {
        stubIds.add(player.id);
      }
    }
  }

  return { ids, stubIds };
}

type BestAccumulator = {
  mapId: number;
  playerId: number;
  time: number;
  finishCount: number;
  firstFinishAt: string;
  bestAt: string;
  splits: number[];
};

export async function applyRaceRows(
  rows: DdnetRaceRow[],
  recordState: RecordState,
  options: {
    dayCounts: 'increment' | 'set';
    newRows?: (row: DdnetRaceRow) => boolean;
    writeState: (tx: TransactionClient, candidates: RecordCandidate[]) => Promise<void>;
  }
) {
  const isNew = options.newRows ?? (() => true);

  const lastSeenByName = new Map<string, Date>();
  for (const row of rows) {
    const seen = lastSeenByName.get(row.playerName);
    if (seen === undefined || row.timestamp > seen) {
      lastSeenByName.set(row.playerName, row.timestamp);
    }
  }

  const mapIds = await resolveMapIds(rows.map((row) => row.mapName));
  const { ids: playerIds } = await resolvePlayerIds(lastSeenByName);

  const bests = new Map<string, BestAccumulator>();
  const playerDays = new Map<string, { day: Date; playerId: number; count: number }>();
  const mapDays = new Map<string, { day: Date; mapId: number; count: number }>();
  const players = new Map<number, { count: number; first: Date; last: Date }>();
  const candidates: RecordCandidate[] = [];

  const sorted = [...rows].sort((a, b) => a.timestamp.getTime() - b.timestamp.getTime());
  const runningRecord = new Map<number, number>();

  for (const row of sorted) {
    const mapId = mapIds.get(row.mapName)!;
    const playerId = playerIds.get(row.playerName);

    if (playerId === undefined) {
      continue;
    }

    const day = utcDay(row.timestamp);
    const dayLabel = day.toISOString().slice(0, 10);

    const playerDayKey = `${dayLabel}:${playerId}`;
    const playerDay = playerDays.get(playerDayKey);
    if (playerDay === undefined) {
      playerDays.set(playerDayKey, { day, playerId, count: 1 });
    } else {
      playerDay.count += 1;
    }

    const mapDayKey = `${dayLabel}:${mapId}`;
    const mapDay = mapDays.get(mapDayKey);
    if (mapDay === undefined) {
      mapDays.set(mapDayKey, { day, mapId, count: 1 });
    } else {
      mapDay.count += 1;
    }

    if (!isNew(row)) {
      continue;
    }

    const bestKey = `${mapId}:${playerId}`;
    const best = bests.get(bestKey);
    if (best === undefined) {
      bests.set(bestKey, {
        mapId,
        playerId,
        time: row.time,
        finishCount: 1,
        firstFinishAt: dayLabel,
        bestAt: dayLabel,
        splits: row.splits,
      });
    } else {
      best.finishCount += 1;
      if (row.time < best.time) {
        best.time = row.time;
        best.bestAt = dayLabel;
        best.splits = row.splits;
      }
    }

    const player = players.get(playerId);
    if (player === undefined) {
      players.set(playerId, { count: 1, first: row.timestamp, last: row.timestamp });
    } else {
      player.count += 1;
      if (row.timestamp < player.first) player.first = row.timestamp;
      if (row.timestamp > player.last) player.last = row.timestamp;
    }

    const record = runningRecord.get(mapId) ?? recordState.get(mapId);
    if (record === undefined || row.time < record) {
      runningRecord.set(mapId, row.time);
      candidates.push({
        mapId,
        playerId,
        time: row.time,
        timestamp: row.timestamp.toISOString(),
      });
    }
  }

  const bestRows = [...bests.values()].sort(
    (a, b) => a.mapId - b.mapId || a.playerId - b.playerId
  );
  const playerDayRows = [...playerDays.values()].sort(
    (a, b) => a.day.getTime() - b.day.getTime() || a.playerId - b.playerId
  );
  const mapDayRows = [...mapDays.values()].sort(
    (a, b) => a.day.getTime() - b.day.getTime() || a.mapId - b.mapId
  );
  const playerRows = [...players.entries()]
    .map(([playerId, agg]) => ({ playerId, ...agg }))
    .sort((a, b) => a.playerId - b.playerId);

  await prisma.$transaction(
    async (tx) => {
      for (const rows_ of chunk(bestRows, INSERT_CHUNK_SIZE)) {
        await tx.$queryRawTyped(upsertDdnetRaceBests(JSON.stringify(rows_)));
      }

      const dayCountSql = {
        increment: { player: incrementPlayerDayFinishes, map: incrementMapDayFinishes },
        set: { player: setPlayerDayFinishes, map: setMapDayFinishes },
      }[options.dayCounts];

      for (const rows_ of chunk(playerDayRows, INSERT_CHUNK_SIZE)) {
        await tx.$queryRawTyped(dayCountSql.player(
          rows_.map((row) => row.day),
          rows_.map((row) => row.playerId),
          rows_.map((row) => row.count)
        ));
      }
      for (const rows_ of chunk(mapDayRows, INSERT_CHUNK_SIZE)) {
        await tx.$queryRawTyped(dayCountSql.map(
          rows_.map((row) => row.day),
          rows_.map((row) => row.mapId),
          rows_.map((row) => row.count)
        ));
      }
      for (const rows_ of chunk(playerRows, INSERT_CHUNK_SIZE)) {
        await tx.$queryRawTyped(incrementDdnetPlayers(
          rows_.map((row) => row.playerId),
          rows_.map((row) => row.count),
          rows_.map((row) => row.first),
          rows_.map((row) => row.last)
        ));
      }

      await options.writeState(tx, candidates);
    },
    { timeout: minutesToMilliseconds(5), maxWait: minutesToMilliseconds(1) }
  );

  return {
    playerIds: playerRows.map((row) => row.playerId),
    mapIds: [...new Set(bestRows.map((row) => row.mapId))],
  };
}

export async function replayRecordCandidates(
  candidates: RecordCandidate[],
  recordState: RecordState,
  tx: TransactionClient
) {
  const events: RecordCandidate[] = [];
  const sorted = [...candidates].sort((a, b) => a.timestamp.localeCompare(b.timestamp));

  for (const candidate of sorted) {
    const record = recordState.get(candidate.mapId);
    if (record === undefined || candidate.time < record) {
      recordState.set(candidate.mapId, candidate.time);
      events.push(candidate);
    }
  }

  for (const events_ of chunk(events, INSERT_CHUNK_SIZE)) {
    await tx.$queryRawTyped(insertDdnetRecords(
      events_.map((event) => event.mapId),
      events_.map((event) => new Date(event.timestamp)),
      events_.map((event) => event.playerId),
      events_.map((event) => event.time)
    ));
  }

  return events.length;
}

export type TeamRun = {
  teamId: string; // hex
  mapName: string;
  time: number;
  timestamp: Date;
  playerNames: string[];
};

export function groupTeamRows(rows: DdnetTeamRaceRow[]): TeamRun[] {
  const teams = new Map<string, TeamRun>();

  for (const row of rows) {
    const team = teams.get(row.teamId);
    if (team === undefined) {
      teams.set(row.teamId, {
        teamId: row.teamId,
        mapName: row.mapName,
        time: row.time,
        timestamp: row.timestamp,
        playerNames: [row.playerName],
      });
    } else if (!team.playerNames.includes(row.playerName)) {
      team.playerNames.push(row.playerName);
    }
  }

  return [...teams.values()];
}

export async function applyTeamRuns(
  teams: TeamRun[],
  options: { writeState: (tx: TransactionClient) => Promise<void> }
) {
  const lastSeenByName = new Map<string, Date>();
  for (const team of teams) {
    for (const name of team.playerNames) {
      const seen = lastSeenByName.get(name);
      if (seen === undefined || team.timestamp > seen) {
        lastSeenByName.set(name, team.timestamp);
      }
    }
  }

  const mapIds = await resolveMapIds(teams.map((team) => team.mapName));
  const { ids: playerIds, stubIds } = await resolvePlayerIds(lastSeenByName);

  const teamRows: { teamId: string; mapId: number; time: number; timestamp: string; size: number }[] = [];
  const rosterRows: { teamId: string; playerId: number }[] = [];
  const partners = new Map<string, {
    playerId: number; partnerId: number; finishCount: number;
    bestTime: number; bestMapId: number; lastFinishAt: Date;
  }>();
  const partnerMaps = new Map<string, {
    playerId: number; partnerId: number; mapId: number;
    bestTime: number; finishCount: number; lastFinishAt: Date;
  }>();

  for (const team of teams) {
    const mapId = mapIds.get(team.mapName)!;
    const memberIds = team.playerNames
      .map((name) => playerIds.get(name))
      .filter((id): id is number => id !== undefined);

    teamRows.push({
      teamId: team.teamId,
      mapId,
      time: team.time,
      timestamp: team.timestamp.toISOString(),
      size: memberIds.length,
    });
    for (const playerId of memberIds) {
      rosterRows.push({ teamId: team.teamId, playerId });
    }

    const day = utcDay(team.timestamp);

    // A stub name is many people, so pairs involving one are meaningless.
    const pairIds = memberIds.filter((id) => !stubIds.has(id));

    for (const playerId of pairIds) {
      for (const partnerId of pairIds) {
        if (playerId === partnerId) {
          continue;
        }

        const key = `${playerId}:${partnerId}`;
        const partner = partners.get(key);
        if (partner === undefined) {
          partners.set(key, {
            playerId, partnerId, finishCount: 1,
            bestTime: team.time, bestMapId: mapId, lastFinishAt: day,
          });
        } else {
          partner.finishCount += 1;
          if (team.time < partner.bestTime) {
            partner.bestTime = team.time;
            partner.bestMapId = mapId;
          }
          if (day > partner.lastFinishAt) partner.lastFinishAt = day;
        }

        const mapKey = `${playerId}:${partnerId}:${mapId}`;
        const partnerMap = partnerMaps.get(mapKey);
        if (partnerMap === undefined) {
          partnerMaps.set(mapKey, {
            playerId, partnerId, mapId,
            bestTime: team.time, finishCount: 1, lastFinishAt: day,
          });
        } else {
          partnerMap.finishCount += 1;
          if (team.time < partnerMap.bestTime) partnerMap.bestTime = team.time;
          if (day > partnerMap.lastFinishAt) partnerMap.lastFinishAt = day;
        }
      }
    }
  }

  const partnerRows = [...partners.values()].sort(
    (a, b) => a.playerId - b.playerId || a.partnerId - b.partnerId
  );
  const partnerMapRows = [...partnerMaps.values()].sort(
    (a, b) => a.playerId - b.playerId || a.partnerId - b.partnerId || a.mapId - b.mapId
  );

  await prisma.$transaction(
    async (tx) => {
      for (const rows_ of chunk(teamRows, INSERT_CHUNK_SIZE)) {
        await tx.$queryRawTyped(upsertDdnetTeamRaces(JSON.stringify(rows_)));
      }
      for (const rows_ of chunk(rosterRows, INSERT_CHUNK_SIZE)) {
        await tx.$queryRawTyped(upsertDdnetTeamRacePlayers(JSON.stringify(rows_)));
      }
      for (const rows_ of chunk(partnerRows, INSERT_CHUNK_SIZE)) {
        await tx.$queryRawTyped(upsertDdnetPartners(
          rows_.map((row) => row.playerId),
          rows_.map((row) => row.partnerId),
          rows_.map((row) => row.finishCount),
          rows_.map((row) => row.bestTime),
          rows_.map((row) => row.bestMapId),
          rows_.map((row) => row.lastFinishAt)
        ));
      }
      for (const rows_ of chunk(partnerMapRows, INSERT_CHUNK_SIZE)) {
        await tx.$queryRawTyped(upsertDdnetPartnerMaps(
          rows_.map((row) => row.playerId),
          rows_.map((row) => row.partnerId),
          rows_.map((row) => row.mapId),
          rows_.map((row) => row.bestTime),
          rows_.map((row) => row.finishCount),
          rows_.map((row) => row.lastFinishAt)
        ));
      }

      await options.writeState(tx);
    },
    { timeout: minutesToMilliseconds(5), maxWait: minutesToMilliseconds(1) }
  );
}
