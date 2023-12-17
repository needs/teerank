import { JobType } from "@prisma/client";
import { prisma } from "./prisma"
import { pollMasterServers } from "./tasks/pollMasterServers";
import { pollGameServers } from "./tasks/pollGameServers";
import { rankPlayers } from "./tasks/rankPlayers";
import { updatePlayTimes } from "./tasks/updatePlayTime";

async function sleep(ms: number) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function main() {
  for (; ;) {
    const job = await prisma.job.findFirst({
      orderBy: {
        priority: 'desc',
      }
    });

    if (job === null) {
      await sleep(5000);
      continue;
    }

    try {
      await prisma.job.delete({
        where: {
          id: job.id,
        }
      });
    } catch (e) {
      continue;
    }

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
  }
}

main()
