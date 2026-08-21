import { GetObjectCommand, ListObjectsV2Command } from "@aws-sdk/client-s3";
import { hoursToMilliseconds } from "date-fns";
import {
  S3_BUCKET,
  SNAPSHOT_RETENTION_HOURS,
  addUtcDays,
  formatUtcDay,
  getEnvInt,
  getS3Client,
  parseUtcDay,
  processRollupBackfillJobs,
} from "@teerank/teerank";
import { prisma } from "../prisma";
import { SnapshotArchiveRow, decodeSnapshotRowsFromParquet } from "../parquet";
import { DayAggregator, RollupSnapshot } from "../rollup/aggregateDay";
import { writeDayRollup } from "../rollup/writeDayRollup";

const ROLLUP_TIME_BUDGET_MS = getEnvInt('ROLLUP_TIME_BUDGET_MS', 10 * 60 * 1000);
const ROLLUP_BACKFILL_DAYS_PER_TICK = getEnvInt('ROLLUP_BACKFILL_DAYS_PER_TICK', 4);

async function listArchivedDays() {
  const s3 = getS3Client();
  const days: string[] = [];
  let continuationToken: string | undefined;

  do {
    const result = await s3.send(new ListObjectsV2Command({
      Bucket: S3_BUCKET,
      Prefix: 'snapshots/',
      Delimiter: '/',
      ContinuationToken: continuationToken,
    }));

    for (const prefix of result.CommonPrefixes ?? []) {
      const match = prefix.Prefix?.match(/dt=(\d{4}-\d{2}-\d{2})\/$/);
      if (match !== null && match !== undefined) {
        days.push(match[1]);
      }
    }

    continuationToken = result.NextContinuationToken;
  } while (continuationToken !== undefined);

  return days.sort();
}

async function listMissingArchivedDays() {
  const days = (await listArchivedDays()).slice(0, -1).filter((day) => {
    const dayEndMs = addUtcDays(parseUtcDay(day), 1).getTime();
    return dayEndMs + hoursToMilliseconds(SNAPSHOT_RETENTION_HOURS) <= Date.now();
  });

  const rolledUpDays = await prisma.playerDay.groupBy({
    by: ['day'],
  });
  const rolledUp = new Set(rolledUpDays.map(({ day }) => formatUtcDay(day)));

  return days.filter((day) => !rolledUp.has(day));
}

async function listDayObjectKeys(day: string) {
  const s3 = getS3Client();
  const keys: string[] = [];
  let continuationToken: string | undefined;

  do {
    const result = await s3.send(new ListObjectsV2Command({
      Bucket: S3_BUCKET,
      Prefix: `snapshots/dt=${day}/`,
      ContinuationToken: continuationToken,
    }));

    for (const object of result.Contents ?? []) {
      if (object.Key !== undefined) {
        keys.push(object.Key);
      }
    }

    continuationToken = result.NextContinuationToken;
  } while (continuationToken !== undefined);

  return keys.sort();
}

// Archive rows are flat client observations; regroup them per snapshot so the
// aggregation sees the same shape as the live rollup.
function addArchiveRows(aggregator: DayAggregator, rows: SnapshotArchiveRow[], day: Date, dayEnd: Date) {
  const snapshots = new Map<number, RollupSnapshot>();

  for (const row of rows) {
    if (row.createdAt < day || row.createdAt >= dayEnd) {
      continue;
    }

    let snapshot = snapshots.get(row.snapshotId);

    if (snapshot === undefined) {
      snapshot = {
        createdAt: row.createdAt,
        gameServerId: row.gameServerId,
        mapId: row.mapId,
        gameTypeName: row.gameTypeName,
        numClients: row.numClients,
        clients: [],
      };
      snapshots.set(row.snapshotId, snapshot);
    }

    if (row.playerName !== null) {
      snapshot.clients.push({
        playerName: row.playerName,
        clanName: row.clanName,
        inGame: row.inGame ?? false,
      });
    }
  }

  for (const snapshot of snapshots.values()) {
    aggregator.addSnapshot(snapshot);
  }
}

async function backfillDay(dayLabel: string) {
  const startedAt = Date.now();
  const day = parseUtcDay(dayLabel);
  const dayEnd = addUtcDays(day, 1);

  const keys = await listDayObjectKeys(dayLabel);

  if (keys.length === 0) {
    console.log(`Backfill for ${dayLabel} skipped: no archive objects`);
    return;
  }

  const s3 = getS3Client();
  const aggregator = new DayAggregator();

  for (const key of keys) {
    if (Date.now() - startedAt > ROLLUP_TIME_BUDGET_MS) {
      throw new Error(`Backfill for ${dayLabel} exceeded time budget, nothing written`);
    }

    const object = await s3.send(new GetObjectCommand({ Bucket: S3_BUCKET, Key: key }));
    const body = await object.Body?.transformToByteArray();

    if (body === undefined) {
      throw new Error(`Archive object ${key} has no body`);
    }

    addArchiveRows(aggregator, decodeSnapshotRowsFromParquet(body), day, dayEnd);
  }

  await writeDayRollup(day, aggregator.finalize());
}

export async function rollupBackfill() {
  const missing = await listMissingArchivedDays();

  for (const day of missing.slice(0, ROLLUP_BACKFILL_DAYS_PER_TICK)) {
    await backfillDay(day);
  }
}

export async function startRollupBackfillWorker() {
  return processRollupBackfillJobs(rollupBackfill);
}
