import { getEnvInt, processDdnetBackfillJobs, wait } from "@teerank/teerank";
import { prisma } from "../prisma";
import {
  DdnetMapInfoRow,
  DdnetMapRow,
  DdnetRaceRow,
  DdnetTeamRaceRow,
  fetchDumpLastModified,
  streamStatsDump,
} from "../ddnet/stream";
import {
  decodeRaceRowsFromParquet,
  decodeTeamRaceRowsFromParquet,
  encodeRaceRowsToParquet,
  encodeTeamRaceRowsToParquet,
} from "../ddnet/raceParquet";
import { putArchiveObject, getArchiveObject, raceArchiveKey, teamRaceArchiveKey } from "../ddnet/archive";
import {
  RecordCandidate,
  applyRaceRows,
  applyTeamRuns,
  ensureFinishPartitions,
  groupTeamRows,
  loadRecordState,
  replayRecordCandidates,
} from "../ddnet/applyChunk";
import { importMapMetadata } from "../ddnet/mapsImport";
import { refreshAllPoints, refreshAllMapStats, refreshPointsRanks } from "../ddnet/refresh";
import {
  BACKFILL_STATE_KEY,
  BoundaryTracker,
  IMPORT_STATE_KEY,
  ImportState,
  getDdnetState,
  monthLabel,
  raceRowKey,
  setDdnetState,
} from "../ddnet/state";

const DDNET_TIME_BUDGET_MS = getEnvInt('DDNET_TIME_BUDGET_MS', 10 * 60 * 1000);
const DDNET_BATCH_PAUSE_MS = getEnvInt('DDNET_BATCH_PAUSE_MS', 200);
const DDNET_BUFFER_ROWS = getEnvInt('DDNET_BUFFER_ROWS', 200_000);

export type BackfillState = {
  phase: 'download' | 'apply' | 'teams' | 'tail' | 'done';
  lastModified?: string;
  raceMonths?: string[];
  raceParts?: Record<string, number>;
  teamMonths?: string[];
  teamParts?: Record<string, number>;
  monthIndex?: number;
  part?: number;
  candidates?: RecordCandidate[];
  raceWatermark?: string;
  teamWatermark?: string;
  raceBoundary?: string[];
  teamBoundary?: string[];
};

// Buffers rows per month and flushes the largest month once the global cap
// is hit, so the unordered 69M-row stream never holds more than the cap.
class MonthBuffers<Row> {
  private buffers = new Map<string, Row[]>();
  private partCounts: Record<string, number> = {};
  private total = 0;

  constructor(
    private encode: (rows: Row[]) => Uint8Array,
    private keyFor: (ym: string, part: string) => string
  ) {}

  async add(ym: string, row: Row) {
    let buffer = this.buffers.get(ym);
    if (buffer === undefined) {
      buffer = [];
      this.buffers.set(ym, buffer);
      this.partCounts[ym] ??= 0;
    }
    buffer.push(row);
    this.total += 1;

    if (this.total >= DDNET_BUFFER_ROWS) {
      let largest: string | null = null;
      for (const [key, rows] of this.buffers) {
        if (largest === null || rows.length > this.buffers.get(largest)!.length) {
          largest = key;
        }
      }
      await this.flush(largest!);
    }
  }

  private async flush(ym: string) {
    const rows = this.buffers.get(ym);
    if (rows === undefined || rows.length === 0) {
      return;
    }

    const part = this.partCounts[ym];
    await putArchiveObject(this.keyFor(ym, `part-${part}`), this.encode(rows));
    this.partCounts[ym] = part + 1;
    this.total -= rows.length;
    this.buffers.set(ym, []);
  }

  async flushAll() {
    for (const ym of this.buffers.keys()) {
      await this.flush(ym);
    }
    return this.partCounts;
  }
}

async function downloadPhase() {
  const lastModified = await fetchDumpLastModified();

  const raceBuffers = new MonthBuffers<DdnetRaceRow>(encodeRaceRowsToParquet, raceArchiveKey);
  const teamBuffers = new MonthBuffers<DdnetTeamRaceRow[]>(
    (teams) => encodeTeamRaceRowsToParquet(teams.flat()),
    teamRaceArchiveKey
  );

  const mapRows: DdnetMapRow[] = [];
  const mapInfoRows: DdnetMapInfoRow[] = [];
  const raceBoundary = new BoundaryTracker();
  const teamBoundary = new BoundaryTracker();

  // A team's rows are contiguous in the CSV and share one timestamp; buffer
  // them as one unit so a flush can never split a team across parts.
  let pendingTeam: DdnetTeamRaceRow[] = [];
  let raceCount = 0;

  const flushPendingTeam = async () => {
    if (pendingTeam.length > 0) {
      await teamBuffers.add(monthLabel(pendingTeam[0].timestamp), pendingTeam);
      teamBoundary.add(pendingTeam[0].teamId, pendingTeam[0].timestamp);
      pendingTeam = [];
    }
  };

  await streamStatsDump({
    onMap: (row) => {
      mapRows.push(row);
    },
    onMapInfo: (row) => {
      mapInfoRows.push(row);
    },
    onRace: async (row) => {
      await raceBuffers.add(monthLabel(row.timestamp), row);
      raceBoundary.add(raceRowKey(row), row.timestamp);
      raceCount += 1;
      if (raceCount % 5_000_000 === 0) {
        console.log(`DDNet backfill download: ${raceCount} race rows streamed`);
      }
    },
    onTeamRace: async (row) => {
      if (pendingTeam.length > 0 && pendingTeam[0].teamId !== row.teamId) {
        await flushPendingTeam();
      }
      pendingTeam.push(row);
    },
  });
  await flushPendingTeam();

  const raceParts = await raceBuffers.flushAll();
  const teamParts = await teamBuffers.flushAll();

  await importMapMetadata(mapRows, mapInfoRows);

  const race = raceBoundary.finalize();
  const team = teamBoundary.finalize();

  const state: BackfillState = {
    phase: 'apply',
    lastModified,
    raceMonths: Object.keys(raceParts).sort(),
    raceParts,
    teamMonths: Object.keys(teamParts).sort(),
    teamParts,
    monthIndex: 0,
    part: -1,
    candidates: [],
    raceWatermark: race.watermark,
    raceBoundary: race.boundary,
    teamWatermark: team.watermark,
    teamBoundary: team.boundary,
  };

  await prisma.$transaction((tx) => setDdnetState(tx, BACKFILL_STATE_KEY, state));
  console.log(
    `DDNet backfill download done: ${raceCount} race rows over ` +
    `${state.raceMonths!.length} months, dump ${lastModified}`
  );
}

async function applyPhase(state: BackfillState, startedAt: number) {
  const recordState = await loadRecordState();

  while (Date.now() - startedAt < DDNET_TIME_BUDGET_MS) {
    const monthIndex = state.monthIndex!;
    const month = state.raceMonths![monthIndex];

    if (month === undefined) {
      state = { ...state, phase: 'teams', monthIndex: 0, part: -1 };
      await prisma.$transaction((tx) => setDdnetState(tx, BACKFILL_STATE_KEY, state));
      return state;
    }

    const partCount = state.raceParts![month];
    const part = state.part! + 1;

    if (part >= partCount) {
      // Month complete: replay its record candidates chronologically.
      const candidates = state.candidates ?? [];
      const next: BackfillState = { ...state, monthIndex: monthIndex + 1, part: -1, candidates: [] };
      await prisma.$transaction(async (tx) => {
        const events = await replayRecordCandidates(candidates, recordState, tx);
        await setDdnetState(tx, BACKFILL_STATE_KEY, next);
        console.log(`DDNet backfill: month ${month} done, ${events} record events`);
      });
      state = next;
      continue;
    }

    await ensureFinishPartitions(month);

    const rows = decodeRaceRowsFromParquet(await getArchiveObject(raceArchiveKey(month, `part-${part}`)));

    await applyRaceRows(rows, recordState, {
      dayCounts: 'increment',
      writeState: async (tx, candidates) => {
        state = { ...state, part, candidates: [...(state.candidates ?? []), ...candidates] };
        await setDdnetState(tx, BACKFILL_STATE_KEY, state);
      },
    });

    console.log(`DDNet backfill: applied race ${month} part ${part + 1}/${partCount} (${rows.length} rows)`);
    await wait(DDNET_BATCH_PAUSE_MS);
  }

  return state;
}

async function teamsPhase(state: BackfillState, startedAt: number) {
  while (Date.now() - startedAt < DDNET_TIME_BUDGET_MS) {
    const monthIndex = state.monthIndex!;
    const month = state.teamMonths![monthIndex];

    if (month === undefined) {
      state = { ...state, phase: 'tail' };
      await prisma.$transaction((tx) => setDdnetState(tx, BACKFILL_STATE_KEY, state));
      return state;
    }

    const partCount = state.teamParts![month];
    const part = state.part! + 1;

    if (part >= partCount) {
      state = { ...state, monthIndex: monthIndex + 1, part: -1 };
      await prisma.$transaction((tx) => setDdnetState(tx, BACKFILL_STATE_KEY, state));
      continue;
    }

    const rows = decodeTeamRaceRowsFromParquet(
      await getArchiveObject(teamRaceArchiveKey(month, `part-${part}`))
    );
    const teams = groupTeamRows(rows);

    await applyTeamRuns(teams, {
      writeState: async (tx) => {
        state = { ...state, part };
        await setDdnetState(tx, BACKFILL_STATE_KEY, state);
      },
    });

    console.log(`DDNet backfill: applied teamrace ${month} part ${part + 1}/${partCount} (${teams.length} teams)`);
    await wait(DDNET_BATCH_PAUSE_MS);
  }

  return state;
}

async function tailPhase(state: BackfillState) {
  console.log('DDNet backfill: tail refresh starting');
  await refreshAllPoints();
  await refreshPointsRanks();
  await refreshAllMapStats();

  const importState: ImportState = {
    lastModified: state.lastModified!,
    raceWatermark: state.raceWatermark!,
    raceBoundary: state.raceBoundary ?? [],
    teamWatermark: state.teamWatermark!,
    teamBoundary: state.teamBoundary ?? [],
    lastFullRefresh: new Date().toISOString(),
  };

  await prisma.$transaction(async (tx) => {
    await setDdnetState(tx, IMPORT_STATE_KEY, importState);
    await setDdnetState(tx, BACKFILL_STATE_KEY, { phase: 'done' } satisfies BackfillState);
  });
  console.log('DDNet backfill: done');
}

export async function ddnetBackfill() {
  const startedAt = Date.now();
  let state = await getDdnetState<BackfillState>(BACKFILL_STATE_KEY) ?? { phase: 'download' as const };

  if (state.phase === 'download') {
    await downloadPhase();
    return;
  }
  if (state.phase === 'apply') {
    state = await applyPhase(state, startedAt);
  }
  if (state.phase === 'teams') {
    state = await teamsPhase(state, startedAt);
  }
  if (state.phase === 'tail') {
    await tailPhase(state);
  }
}

export async function startDdnetBackfillWorker() {
  return processDdnetBackfillJobs(ddnetBackfill);
}
