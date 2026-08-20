import { GetObjectCommand, ListObjectsV2Command } from "@aws-sdk/client-s3";
import { hoursToMilliseconds } from "date-fns";
import {
  RollupBackfillJobData,
  S3_BUCKET,
  SNAPSHOT_RETENTION_HOURS,
  addUtcDays,
  getEnvInt,
  getS3Client,
  parseUtcDay,
  processRollupBackfillJobs,
} from "@teerank/teerank";
import { SnapshotArchiveRow, decodeSnapshotRowsFromParquet } from "../parquet";
import { DayAggregator, RollupSnapshot } from "../rollup/aggregateDay";
import { writeDayRollup } from "../rollup/writeDayRollup";
import { isDayRolledUp } from "./rollupDay";

const ROLLUP_TIME_BUDGET_MS = getEnvInt('ROLLUP_TIME_BUDGET_MS', 10 * 60 * 1000);

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

export async function rollupBackfill(data: RollupBackfillJobData) {
  const startedAt = Date.now();
  const day = parseUtcDay(data.day);
  const dayEnd = addUtcDays(day, 1);

  // The archive only holds all of a day's snapshots once the retention window
  // has moved past the day's end.
  if (dayEnd.getTime() + hoursToMilliseconds(SNAPSHOT_RETENTION_HOURS) > Date.now()) {
    console.log(`Backfill for ${data.day} skipped: day may not be fully archived`);
    return;
  }

  if (await isDayRolledUp(day)) {
    console.log(`Backfill for ${data.day} skipped: already rolled up`);
    return;
  }

  const keys = await listDayObjectKeys(data.day);

  if (keys.length === 0) {
    console.log(`Backfill for ${data.day} skipped: no archive objects`);
    return;
  }

  const s3 = getS3Client();
  const aggregator = new DayAggregator();

  for (const key of keys) {
    if (Date.now() - startedAt > ROLLUP_TIME_BUDGET_MS) {
      throw new Error(`Backfill for ${data.day} exceeded time budget, nothing written`);
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

export async function startRollupBackfillWorker() {
  return processRollupBackfillJobs(rollupBackfill);
}
