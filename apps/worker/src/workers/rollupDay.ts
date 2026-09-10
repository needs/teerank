import {
  RollupDayJobData,
  addUtcDays,
  getEnvInt,
  parseUtcDay,
  processRollupDayJobs,
} from "@teerank/teerank";
import { rollupPrisma } from "../prisma";
import { iterateSnapshots } from "../snapshots";
import { DayAggregator } from "../rollup/aggregateDay";
import { writeDayRollup } from "../rollup/writeDayRollup";

const ROLLUP_BATCH_SIZE = getEnvInt('ROLLUP_BATCH_SIZE', 2000);
const ROLLUP_TIME_BUDGET_MS = getEnvInt('ROLLUP_TIME_BUDGET_MS', 10 * 60 * 1000);

export async function isDayRolledUp(day: Date) {
  const existing = await rollupPrisma.globalDay.findUnique({
    where: { day },
    select: { day: true },
  });

  return existing !== null;
}

export async function rollupDay(data: RollupDayJobData) {
  const startedAt = Date.now();
  const day = parseUtcDay(data.day);
  const dayEnd = addUtcDays(day, 1);

  if (dayEnd.getTime() > Date.now()) {
    console.log(`Rollup for ${data.day} skipped: day is not over`);
    return;
  }

  if (await isDayRolledUp(day)) {
    console.log(`Rollup for ${data.day} skipped: already rolled up`);
    return;
  }

  const aggregator = new DayAggregator();
  let snapshotCount = 0;

  for await (const snapshot of iterateSnapshots({
    from: day,
    to: dayEnd,
    batchSize: ROLLUP_BATCH_SIZE,
    prisma: rollupPrisma,
  })) {
    if (Date.now() - startedAt > ROLLUP_TIME_BUDGET_MS) {
      throw new Error(
        `Rollup for ${data.day} exceeded time budget after ${snapshotCount} snapshots, nothing written`
      );
    }

    snapshotCount += 1;
    if (snapshotCount % 50_000 === 0) {
      console.log(
        `Rollup for ${data.day}: ${snapshotCount} snapshots read in ${Math.round((Date.now() - startedAt) / 1000)}s`
      );
    }

    aggregator.addSnapshot({
      createdAt: snapshot.createdAt,
      gameServerId: snapshot.gameServerId,
      mapId: snapshot.mapId,
      gameTypeName: snapshot.map.gameTypeName,
      numClients: snapshot.numClients,
      clients: snapshot.clients,
    });
  }

  await writeDayRollup(day, aggregator.finalize());
}

export async function startRollupDayWorker() {
  return processRollupDayJobs(rollupDay);
}
