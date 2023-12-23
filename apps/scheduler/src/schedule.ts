import { JobType } from "@prisma/client";
import { prisma } from "./prisma";

export function jobPriority(jobType: JobType) {
  switch (jobType) {
    case JobType.POLL_MASTER_SERVERS:
      return 4;
    case JobType.POLL_GAME_SERVERS:
      return 3;
    case JobType.RANK_PLAYERS:
      return 2;
    case JobType.UPDATE_PLAYTIME:
      return 1;
    case JobType.INITIALIZE_PLAYER_LAST_GAME_SERVER_CLIENT:
      return 0;
  }
}

// In some cases, like when playtimes are reset, a lot of job will be scheduled
// at once, which might overload the database.
export const MAXIMUM_JOB_PER_SCHEDULE = 100;

export async function scheduleJobs(jobType: JobType, batchSize: number, minRange: number, maxRange: number, maximumScheduledJobs: number = MAXIMUM_JOB_PER_SCHEDULE) {
  const batchStart = minRange - (minRange % batchSize);
  const batchEnd = Math.min(maxRange - (maxRange % batchSize) + batchSize, batchStart + maximumScheduledJobs * batchSize);

  const results = await prisma.job.createMany({
    data: Array.from({ length: (batchEnd - batchStart) / batchSize }, (_, i) => ({
      jobType,
      rangeStart: batchStart + i * batchSize,
      rangeEnd: batchStart + (i + 1) * batchSize - 1,
      priority: jobPriority(jobType),
    })),
    skipDuplicates: true,
  });

  if (results.count !== 0) {
    console.log(`Scheduled ${(results.count)} jobs for ${jobType} (${batchStart} - ${batchEnd})`)
  }
}

export async function scheduleJobBatches(jobType: JobType, batchSize: number, batches: number[]) {
  const batchRanges = batches.slice(0, MAXIMUM_JOB_PER_SCHEDULE).map(batch => ({
    rangeStart: batch - (batch % batchSize),
    rangeEnd: batch - (batch % batchSize) + batchSize - 1,
  }));

  const results = await prisma.job.createMany({
    data: batchRanges.map(range => ({
      jobType,
      rangeStart: range.rangeStart,
      rangeEnd: range.rangeEnd,
      priority: jobPriority(jobType),
    })),
    skipDuplicates: true,
  });

  if (results.count !== 0) {
    console.log(`Scheduled a batch of ${(results.count)} jobs for ${jobType}`)
  }
}
