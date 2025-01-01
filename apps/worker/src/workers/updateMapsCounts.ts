import { Worker } from "bullmq";
import { prisma } from "../prisma";
import { bullmqConnection, QUEUE_NAME_MAP_COUNT } from "@teerank/teerank"
import { getEnvInt } from "@teerank/teerank";
import { minutesToSeconds } from "date-fns";

const UPDATE_MAPS_COUNTS_CONCURRENCY = getEnvInt('UPDATE_MAPS_COUNTS_CONCURRENCY', 5);

export async function updateMapsCount(gameTypeName: string, mapName: string) {
  const [map, gameServerCount] = await Promise.all([
    prisma.map.findUniqueOrThrow({
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
          name: mapName,
          gameTypeName,
        },
      },
    }),

    prisma.gameServerState.count({
      where: {
        map: {
          name: mapName,
          gameTypeName,
        },
      },
    })
  ]);

  await prisma.map.update({
    where: {
      name_gameTypeName: {
        name: mapName,
        gameTypeName,
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
  return new Worker(QUEUE_NAME_MAP_COUNT, job => updateMapsCount(job.data.gameTypeName, job.data.mapName), {
    connection: bullmqConnection,
    concurrency: UPDATE_MAPS_COUNTS_CONCURRENCY,
    removeOnComplete: {
      age: minutesToSeconds(10),
    },
    removeOnFail: {
      count: 1000,
    }
  });
}
