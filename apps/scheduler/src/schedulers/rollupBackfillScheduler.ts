import { ListObjectsV2Command } from "@aws-sdk/client-s3";
import { hoursToMilliseconds, minutesToMilliseconds } from "date-fns";
import {
  S3_BUCKET,
  SNAPSHOT_RETENTION_HOURS,
  addUtcDays,
  formatUtcDay,
  getEnvInt,
  getS3Client,
  parseUtcDay,
  scheduleRollupBackfill,
} from "@teerank/teerank";
import { schedule } from "../utils";
import { prisma } from "../prisma";

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

export function rollupBackfillScheduler() {
  schedule(minutesToMilliseconds(15), async () => {
    const days = (await listArchivedDays()).slice(0, -1).filter((day) => {
      const dayEndMs = addUtcDays(parseUtcDay(day), 1).getTime();
      return dayEndMs + hoursToMilliseconds(SNAPSHOT_RETENTION_HOURS) <= Date.now();
    });

    if (days.length === 0) {
      return;
    }

    const rolledUpDays = await prisma.playerDay.groupBy({
      by: ['day'],
    });
    const rolledUp = new Set(rolledUpDays.map(({ day }) => formatUtcDay(day)));

    const missing = days.filter((day) => !rolledUp.has(day)).slice(0, ROLLUP_BACKFILL_DAYS_PER_TICK);

    for (const day of missing) {
      await scheduleRollupBackfill({ day });
    }
  });
}
