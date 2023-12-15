import { JobType } from "@prisma/client";
import { prisma } from "./prisma";

function jobPriority(jobType: JobType) {
  switch (jobType) {
    case JobType.POLL_MASTER_SERVERS:
      return 1;
    case JobType.POLL_GAME_SERVERS:
      return 2;
    case JobType.RANK_PLAYERS:
      return 3;
    case JobType.UPDATE_PLAYTIME:
      return 4;
  }
}

export async function scheduleJobs(jobType: JobType, batchSize: number, minRange: number, maxRange: number) {
  const batchStart = minRange - (minRange % batchSize);
  const batchEnd = maxRange - (maxRange % batchSize) + batchSize;

  // Schedule job one by one to not overload database  when ther are too many
  // batches to schedule.
  for (let i = batchStart; i < batchEnd; i += batchSize) {
    await prisma.job.create({
      data: {
        jobType,
        rangeStart: i,
        rangeEnd: i + batchSize,
        priority: jobPriority(jobType),
      },
    });
  }
}
