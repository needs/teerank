import { JobType } from "@prisma/client";
import { prisma } from "../prisma";
import { scheduleJobs } from "../schedule";

const BATCH_SIZE = 100;

export async function schedulePlayerLastGameServerClientInitialization() {
  const uninitializedPlayerCount = await prisma.player.count({
    where: {
      lastGameServerClientInitializedAt: null,
    },
  });

  if (uninitializedPlayerCount > 0) {
    await scheduleJobs(JobType.INITIALIZE_PLAYER_LAST_GAME_SERVER_CLIENT, BATCH_SIZE, 0, uninitializedPlayerCount);
  }
}
