import { differenceInDays } from "date-fns";
import { processDdnetImportJobs } from "@teerank/teerank";
import {
  DdnetMapInfoRow,
  DdnetMapRow,
  DdnetRaceRow,
  DdnetTeamRaceRow,
  fetchDumpLastModified,
  streamStatsDump,
} from "../ddnet/stream";
import { encodeRaceRowsToParquet, encodeTeamRaceRowsToParquet } from "../ddnet/raceParquet";
import { putArchiveObject, raceArchiveKey, teamRaceArchiveKey } from "../ddnet/archive";
import {
  applyRaceRows,
  applyTeamRuns,
  ensureFinishPartitions,
  groupTeamRows,
  loadRecordState,
  replayRecordCandidates,
} from "../ddnet/applyChunk";
import { importMapMetadata, syncMapThumbnails } from "../ddnet/mapsImport";
import {
  refreshAllPoints,
  refreshAllMapStats,
  refreshMapStats,
  refreshPointsForMaps,
  refreshPointsForPlayers,
  refreshPointsRanks,
} from "../ddnet/refresh";
import {
  BACKFILL_STATE_KEY,
  BOUNDARY_WINDOW_MS,
  BoundaryTracker,
  IMPORT_STATE_KEY,
  ImportState,
  getDdnetState,
  monthLabel,
  raceRowKey,
  setDdnetState,
} from "../ddnet/state";
import { prisma } from "../prisma";

// Whole trailing days are re-counted on every run, which both covers late
// rows near the dump cut and heals finish counts wiped by a re-rollup.
const RECOUNT_DAYS = 3;

function startOfRecountWindow(watermark: Date) {
  const day = new Date(Date.UTC(
    watermark.getUTCFullYear(), watermark.getUTCMonth(), watermark.getUTCDate()
  ));
  day.setUTCDate(day.getUTCDate() - (RECOUNT_DAYS - 1));
  return day;
}

async function archiveNewRows(
  raceRows: DdnetRaceRow[],
  teamRows: DdnetTeamRaceRow[],
  dumpLabel: string
) {
  const raceByMonth = new Map<string, DdnetRaceRow[]>();
  for (const row of raceRows) {
    const ym = monthLabel(row.timestamp);
    (raceByMonth.get(ym) ?? raceByMonth.set(ym, []).get(ym)!).push(row);
  }
  for (const [ym, rows] of raceByMonth) {
    await putArchiveObject(raceArchiveKey(ym, `dump-${dumpLabel}`), encodeRaceRowsToParquet(rows));
  }

  const teamByMonth = new Map<string, DdnetTeamRaceRow[]>();
  for (const row of teamRows) {
    const ym = monthLabel(row.timestamp);
    (teamByMonth.get(ym) ?? teamByMonth.set(ym, []).get(ym)!).push(row);
  }
  for (const [ym, rows] of teamByMonth) {
    await putArchiveObject(teamRaceArchiveKey(ym, `dump-${dumpLabel}`), encodeTeamRaceRowsToParquet(rows));
  }
}

export async function ddnetImport() {
  const backfill = await getDdnetState<{ phase: string }>(BACKFILL_STATE_KEY);
  if (backfill?.phase !== 'done') {
    return;
  }

  const state = await getDdnetState<ImportState>(IMPORT_STATE_KEY);
  if (state === null) {
    throw new Error('DDNet import state missing while backfill is done');
  }

  const lastModified = await fetchDumpLastModified();
  if (lastModified === state.lastModified) {
    await syncMapThumbnails();
    return;
  }

  const raceWatermark = new Date(state.raceWatermark);
  const teamWatermark = new Date(state.teamWatermark);
  const windowStart = startOfRecountWindow(raceWatermark);
  const raceBoundary = new Set(state.raceBoundary);
  const teamBoundary = new Set(state.teamBoundary);

  const mapRows: DdnetMapRow[] = [];
  const mapInfoRows: DdnetMapInfoRow[] = [];
  const windowRows: DdnetRaceRow[] = [];
  const teamWindowRows: DdnetTeamRaceRow[] = [];

  const raceTracker = new BoundaryTracker();
  const teamTracker = new BoundaryTracker();

  await streamStatsDump({
    onMap: (row) => {
      mapRows.push(row);
    },
    onMapInfo: (row) => {
      mapInfoRows.push(row);
    },
    onRace: (row) => {
      if (row.timestamp >= windowStart) {
        windowRows.push(row);
        raceTracker.add(raceRowKey(row), row.timestamp);
      }
    },
    onTeamRace: (row) => {
      if (row.timestamp.getTime() > teamWatermark.getTime() - BOUNDARY_WINDOW_MS) {
        teamWindowRows.push(row);
      }
    },
  });

  const isNewRace = (row: DdnetRaceRow) => {
    const ms = row.timestamp.getTime();
    if (ms > raceWatermark.getTime()) {
      return true;
    }
    return ms > raceWatermark.getTime() - BOUNDARY_WINDOW_MS && !raceBoundary.has(raceRowKey(row));
  };

  const newRaceRows = windowRows.filter(isNewRace);

  const teams = groupTeamRows(teamWindowRows);
  const newTeams = teams.filter((team) => {
    const ms = team.timestamp.getTime();
    if (ms > teamWatermark.getTime()) {
      return true;
    }
    return !teamBoundary.has(team.teamId);
  });
  for (const team of teams) {
    teamTracker.add(team.teamId, team.timestamp);
  }

  const dumpLabel = new Date(lastModified).toISOString().slice(0, 10);
  await archiveNewRows(newRaceRows, teamWindowRows.filter((row) =>
    newTeams.some((team) => team.teamId === row.teamId)
  ), dumpLabel);

  const { pointsChangedMapIds } = await importMapMetadata(mapRows, mapInfoRows);

  for (const ym of new Set(windowRows.map((row) => monthLabel(row.timestamp)))) {
    await ensureFinishPartitions(ym);
  }

  const recordState = await loadRecordState();
  const race = raceTracker.finalize();

  const applied = await applyRaceRows(windowRows, recordState, {
    dayCounts: 'set',
    newRows: isNewRace,
    writeState: async (tx, candidates) => {
      await replayRecordCandidates(candidates, recordState, tx);
      await setDdnetState(tx, IMPORT_STATE_KEY, {
        ...state,
        raceWatermark: race.watermark,
        raceBoundary: race.boundary,
      } satisfies ImportState);
    },
  });

  const team = teamTracker.finalize();

  await applyTeamRuns(newTeams, {
    writeState: async (tx) => {
      await setDdnetState(tx, IMPORT_STATE_KEY, {
        ...state,
        raceWatermark: race.watermark,
        raceBoundary: race.boundary,
        teamWatermark: team.watermark,
        teamBoundary: team.boundary,
        lastModified,
      } satisfies ImportState);
    },
  });

  await refreshPointsForPlayers(applied.playerIds);
  await refreshPointsForMaps(pointsChangedMapIds);
  await refreshMapStats(applied.mapIds);
  await refreshPointsRanks();

  if (differenceInDays(new Date(), new Date(state.lastFullRefresh)) >= 7) {
    console.log('DDNet import: weekly full refresh');
    await refreshAllPoints();
    await refreshPointsRanks();
    await refreshAllMapStats();
    await prisma.$transaction((tx) => setDdnetState(tx, IMPORT_STATE_KEY, {
      ...state,
      raceWatermark: race.watermark,
      raceBoundary: race.boundary,
      teamWatermark: team.watermark,
      teamBoundary: team.boundary,
      lastModified,
      lastFullRefresh: new Date().toISOString(),
    } satisfies ImportState));
  }

  await syncMapThumbnails();

  console.log(
    `DDNet import: dump ${dumpLabel} applied — ${newRaceRows.length} new finishes, ` +
    `${newTeams.length} new team runs, ${pointsChangedMapIds.length} maps re-pointed`
  );
}

export async function startDdnetImportWorker() {
  return processDdnetImportJobs(ddnetImport);
}
