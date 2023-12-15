import { JobType } from "@prisma/client";
import { prisma } from "../prisma";
import { scheduleJobs } from "../schedule";

const BATCH_SIZE = 100;

export async function scheduleGameServersPolling() {
  const results = await prisma.gameServer.aggregate({
    _min: {
      id: true
    },
    _max: {
      id: true
    },
  });

  if (results._min.id !== null && results._max.id !== null) {
    await scheduleJobs(JobType.POLL_GAME_SERVERS, BATCH_SIZE, results._min.id, results._max.id);
  }
}
