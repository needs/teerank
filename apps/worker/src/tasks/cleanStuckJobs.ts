import { subMinutes } from "date-fns";
import { prisma } from "../prisma";

export const jobTimeoutMinutes = 15;

export async function cleanStuckJobs() {
  const unstuckMasterServerPollingJobs = await prisma.masterServer.updateMany({
    where: {
      pollingStartedAt: {
        lt: subMinutes(new Date(), jobTimeoutMinutes),
      },
    },
    data: {
      pollingStartedAt: null,
    },
  });

  if (unstuckMasterServerPollingJobs.count > 0 && process.env.NODE_ENV !== 'test') {
    console.warn(`Unstuck ${unstuckMasterServerPollingJobs.count} master server polling jobs`);
  }

  const unstuckGameServerPollingJobs = await prisma.gameServer.updateMany({
    where: {
      pollingStartedAt: {
        lt: subMinutes(new Date(), jobTimeoutMinutes),
      },
    },
    data: {
      pollingStartedAt: null,
    },
  });

  if (unstuckGameServerPollingJobs.count > 0 && process.env.NODE_ENV !== 'test') {
    console.warn(`Unstuck ${unstuckGameServerPollingJobs.count} game server polling jobs`);
  }

  const unstuckSnapshotRankingJobs = await prisma.gameServerSnapshot.updateMany({
    where: {
      rankingStartedAt: {
        lt: subMinutes(new Date(), jobTimeoutMinutes),
      },
      rankedAt: null,
    },
    data: {
      rankingStartedAt: null,
    },
  });

  if (unstuckSnapshotRankingJobs.count > 0 && process.env.NODE_ENV !== 'test') {
    console.warn(`Unstuck ${unstuckSnapshotRankingJobs.count} snapshot ranking jobs`);
  }

  const unstuckSnapshotPlayTimingJobs = await prisma.gameServerSnapshot.updateMany({
    where: {
      playTimingStartedAt: {
        lt: subMinutes(new Date(), jobTimeoutMinutes),
      },
      playTimedAt: null,
    },
    data: {
      playTimingStartedAt: null,
    },
  });

  if (unstuckSnapshotPlayTimingJobs.count > 0 && process.env.NODE_ENV !== 'test') {
    console.warn(`Unstuck ${unstuckSnapshotPlayTimingJobs.count} snapshot play timing jobs`);
  }

  return unstuckGameServerPollingJobs.count > 0 || unstuckSnapshotRankingJobs.count > 0 || unstuckSnapshotPlayTimingJobs.count > 0 || unstuckMasterServerPollingJobs.count > 0;
}
