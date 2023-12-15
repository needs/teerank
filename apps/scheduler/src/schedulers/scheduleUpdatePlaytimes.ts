import { JobType } from "@prisma/client";
import { prisma } from "../prisma";
import { scheduleJobs } from "../schedule";

const BATCH_SIZE = 100;

export async function scheduleUpdatePlaytimes() {
  const results = await prisma.gameServerSnapshot.aggregate({
    _min: {
      id: true
    },
    _max: {
      id: true
    },
    where: {
      playTimedAt: null,
    }
  });

  if (results._min.id !== null && results._max.id !== null) {
    await scheduleJobs(JobType.UPDATE_PLAYTIME, BATCH_SIZE, results._min.id, results._max.id);
  }
}
