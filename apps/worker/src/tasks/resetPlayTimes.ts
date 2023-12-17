import { TaskRunStatus } from ".prisma/client";
import { Task } from "../task";
import { prisma } from "../prisma";

export const resetPlayTimes: Task = async () => {
  await prisma.clanInfoGameType.updateMany({
    data: {
      playTime: 0,
    },
  });

  await prisma.clanInfoMap.updateMany({
    data: {
      playTime: 0,
    },
  });

  await prisma.playerInfoGameType.updateMany({
    data: {
      playTime: 0,
    },
  });

  await prisma.playerInfoMap.updateMany({
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

  await prisma.gameType.updateMany({
    data: {
      playTime: 0,
    },
  });

  await prisma.map.updateMany({
    data: {
      playTime: 0,
    },
  });

  await prisma.clanPlayerInfo.updateMany({
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
