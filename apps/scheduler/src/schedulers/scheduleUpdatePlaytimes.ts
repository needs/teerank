import { JobType } from "@prisma/client";
import { prisma } from "../prisma";
import { MAXIMUM_JOB_PER_SCHEDULE, scheduleJobBatches } from "../schedule";

const BATCH_SIZE = 100;

export async function scheduleUpdatePlaytimes() {
  const mostRecentUnrankedSnapshot = await prisma.gameServerSnapshot.findFirst({
    select: {
      id: true
    },
    where: {
      playTimedAt: null,
    },
    orderBy: {
      createdAt: 'desc',
    },
  });

  if (mostRecentUnrankedSnapshot !== null) {
    const batches = Array.from({ length: MAXIMUM_JOB_PER_SCHEDULE }, (_, index) => mostRecentUnrankedSnapshot.id - index * BATCH_SIZE);
    await scheduleJobBatches(JobType.UPDATE_PLAYTIME, BATCH_SIZE, batches);
  }
}
