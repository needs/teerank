import { JobType } from "@prisma/client";
import { prisma } from "./prisma"
import { pollMasterServers } from "./tasks/pollMasterServers";
import { pollGameServers } from "./tasks/pollGameServers";
import { rankPlayers } from "./tasks/rankPlayers";
import { updatePlayTimes } from "./tasks/updatePlayTime";
import { secondsToMilliseconds } from "date-fns";

async function sleep(ms: number) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function main() {
  for (; ;) {
    const job = await prisma.job.findFirst({
      where: {
        startedAt: null,
      },
      orderBy: {
        priority: 'desc',
      }
    });

    if (job === null) {
      await sleep(secondsToMilliseconds(5));
      continue;
    }

    try {
      await prisma.job.update({
        where: {
          id: job.id,
          startedAt: null,
        },
        data: {
          startedAt: new Date(),
        }
      });
    } catch (e) {
      console.log(e);
      continue;
    }

    console.time(`Job ${job.jobType} (${job.rangeStart} - ${job.rangeEnd})`);

    switch (job.jobType) {
      case JobType.POLL_MASTER_SERVERS:
        await pollMasterServers(job.rangeStart, job.rangeEnd);
        break;
      case JobType.POLL_GAME_SERVERS:
        await pollGameServers(job.rangeStart, job.rangeEnd);
        break;
      case JobType.RANK_PLAYERS:
        await rankPlayers(job.rangeStart, job.rangeEnd);
        break;
      case JobType.UPDATE_PLAYTIME:
        await updatePlayTimes(job.rangeStart, job.rangeEnd);
        break;
    }

    console.timeEnd(`Job ${job.jobType} (${job.rangeStart} - ${job.rangeEnd})`);

    try {
      await prisma.job.delete({
        where: {
          id: job.id,
        }
      });
    } catch (error) {
      console.error(error);
      console.error(`Failed to delete job ${job.id}, this could happend when job is taking too long to complete`);
      continue;
    }
  }
}

main()
