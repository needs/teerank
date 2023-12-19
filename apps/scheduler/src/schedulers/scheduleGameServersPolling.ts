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

  const count = await prisma.gameServer.count();

  await scheduleJobs(JobType.POLL_GAME_SERVERS, BATCH_SIZE, 0, count);
}
