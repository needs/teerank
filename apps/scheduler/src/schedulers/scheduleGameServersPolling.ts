import { JobType } from "@prisma/client";
import { prisma } from "../prisma";
import { scheduleJobs } from "../schedule";
import { subMinutes } from "date-fns";

const BATCH_SIZE = 100;

export async function scheduleGameServersPolling() {
  const cooldownCount = await prisma.gameServer.count({
    where: {
      polledAt: {
        gte: subMinutes(new Date(), 5),
      }
    }
  });

  if (cooldownCount > 0) {
    return;
  }

  const results = await prisma.gameServer.aggregate({
    _min: {
      id: true
    },
    _max: {
      id: true
    },
    where: {
      OR: [{
        polledAt: {
          lt: subMinutes(new Date(), 5),
        }
      }, {
        polledAt: null,
      }]
    }
  });

  if (results._min.id !== null && results._max.id !== null) {
    await scheduleJobs(JobType.POLL_GAME_SERVERS, BATCH_SIZE, results._min.id, results._max.id);
  }
}
