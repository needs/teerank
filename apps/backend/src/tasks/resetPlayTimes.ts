import { TaskRunStatus } from ".prisma/client";
import { Task } from "../task";
import { prisma } from "../prisma";

export const resetPlayTimes: Task = async () => {
  await prisma.clanInfo.updateMany({
    data: {
      playTime: 0,
    },
  });

  await prisma.playerInfo.updateMany({
    data: {
      playTime: 0,
    },
  });

  await prisma.player.updateMany({
    data: {
      playTime: 0,
    },
  });

  await prisma.clan.updateMany({
    data: {
      playTime: 0,
    },
  });

  await prisma.gameServerSnapshot.updateMany({
    data: {
      playTimedAt: null,
    },
  });

  return TaskRunStatus.COMPLETED;
}
