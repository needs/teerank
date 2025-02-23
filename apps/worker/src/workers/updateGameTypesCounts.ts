import { prisma } from "../prisma";
import { GameTypeCountJobData, processGameTypeCountJobs } from "@teerank/teerank"

export async function updateGameTypeCount(data: GameTypeCountJobData) {
  const gameType = await prisma.gameType.findUniqueOrThrow({
    select: {
      _count: {
        select: {
          playerInfoGameTypes: true,
          map: true,
          clanInfoGameTypes: true,
        },
      },
    },
    where: {
      name: data.gameTypeName,
    },
  });

  const gameServerCount = await prisma.gameServerState.count({
    where: {
      map: {
        gameTypeName: data.gameTypeName,
      },
    },
  });

  await prisma.gameType.update({
    where: {
      name: data.gameTypeName,
    },
    data: {
      playerCount: gameType._count.playerInfoGameTypes,
      mapCount: gameType._count.map,
      clanCount: gameType._count.clanInfoGameTypes,
      gameServerCount,
    },
  });
}

export async function startUpdateGameTypesCountsWorker() {
  return await processGameTypeCountJobs(updateGameTypeCount);
}
