import {
  RollupDayJobData,
  addUtcDays,
  getEnvInt,
  parseUtcDay,
  processRollupDayJobs,
} from "@teerank/teerank";
import { prisma } from "../prisma";
import { iterateSnapshots } from "../snapshots";
import { DayAggregator } from "../rollup/aggregateDay";
import { writeDayRollup } from "../rollup/writeDayRollup";

const ROLLUP_BATCH_SIZE = getEnvInt('ROLLUP_BATCH_SIZE', 2000);
const ROLLUP_TIME_BUDGET_MS = getEnvInt('ROLLUP_TIME_BUDGET_MS', 10 * 60 * 1000);

export async function isDayRolledUp(day: Date) {
  const existing = await prisma.playerDay.findFirst({
    where: { day },
    select: { playerId: true },
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

  for await (const snapshot of iterateSnapshots({
    from: day,
    to: dayEnd,
    batchSize: ROLLUP_BATCH_SIZE,
  })) {
    if (Date.now() - startedAt > ROLLUP_TIME_BUDGET_MS) {
      throw new Error(`Rollup for ${data.day} exceeded time budget, nothing written`);
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
