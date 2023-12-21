import { JobType } from "@prisma/client";
import { prisma } from "../prisma";
import { MAXIMUM_JOB_PER_SCHEDULE, scheduleJobBatches } from "../schedule";

const BATCH_SIZE = 100;

export async function schedulePlayerRanking() {
  const mostRecentUnrankedSnapshot = await prisma.gameServerSnapshot.findFirst({
    select: {
      id: true
    },
    where: {
      rankedAt: null,
    },
    orderBy: {
      createdAt: 'desc',
    },
  });

  if (mostRecentUnrankedSnapshot !== null) {
    const batches = Array.from({ length: MAXIMUM_JOB_PER_SCHEDULE }, (_, index) => mostRecentUnrankedSnapshot.id - index * BATCH_SIZE);
    await scheduleJobBatches(JobType.RANK_PLAYERS, BATCH_SIZE, batches);
  }
}
