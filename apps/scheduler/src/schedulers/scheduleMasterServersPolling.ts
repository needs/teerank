import { JobType } from "@prisma/client";
import { prisma } from "../prisma";
import { scheduleJobs } from "../schedule";
import { subMinutes } from "date-fns";

const BATCH_SIZE = 100;

export async function scheduleMasterServersPolling() {
  const results = await prisma.masterServer.aggregate({
    _min: {
      id: true
    },
    _max: {
      id: true
    },
    where: {
      polledAt: {
        lt: subMinutes(new Date(), 10)
      },
    }
  });

  if (results._min.id !== null && results._max.id !== null) {
    await scheduleJobs(JobType.POLL_MASTER_SERVERS, BATCH_SIZE, results._min.id, results._max.id);
  }
}
