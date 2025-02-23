import { prisma } from "../prisma";
import { MapCountJobData, processMapCountJobs } from "@teerank/teerank"

export async function updateMapsCount(data: MapCountJobData) {
  const map = await prisma.map.findUniqueOrThrow({
    select: {
      _count: {
        select: {
          playerInfoMaps: true,
          clanInfoMaps: true,
        },
      },
    },
    where: {
      name_gameTypeName: {
        name: data.mapName,
        gameTypeName: data.gameTypeName,
      },
    },
  });

  const gameServerCount = await prisma.gameServerState.count({
    where: {
      map: {
        name: data.mapName,
        gameTypeName: data.gameTypeName,
      },
    },
  });

  await prisma.map.update({
    where: {
      name_gameTypeName: {
        name: data.mapName,
        gameTypeName: data.gameTypeName,
      },
    },
    data: {
      playerCount: map._count.playerInfoMaps,
      clanCount: map._count.clanInfoMaps,
      gameServerCount,
    },
  });
}

export async function startUpdateMapsCountsWorker() {
  return await processMapCountJobs(updateMapsCount);
}
