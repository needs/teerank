import { Worker } from "bullmq";
import { prisma } from "../prisma";
import { bullmqConnection, QUEUE_NAME_GAME_TYPE_COUNT } from "@teerank/teerank"
import { getEnvInt } from "@teerank/teerank";
import { minutesToSeconds } from "date-fns";

const UPDATE_GAME_TYPES_COUNTS_CONCURRENCY = getEnvInt('UPDATE_GAME_TYPES_COUNTS_CONCURRENCY', 5);

export async function updateGameTypeCount(gameTypeName: string) {
  const [gameType, gameServerCount] = await Promise.all([
    prisma.gameType.findUniqueOrThrow({
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
        name: gameTypeName,
      },
    }),

    prisma.gameServerState.count({
      where: {
        map: {
          gameTypeName,
        },
      },
    })
  ]);

  await prisma.gameType.update({
    where: {
      name: gameTypeName,
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
  return new Worker(QUEUE_NAME_GAME_TYPE_COUNT, job => updateGameTypeCount(job.data.gameTypeName), {
    connection: bullmqConnection,
    concurrency: UPDATE_GAME_TYPES_COUNTS_CONCURRENCY,
    removeOnComplete: {
      age: minutesToSeconds(10),
    },
    removeOnFail: {
      count: 1000,
    }
  });
}
