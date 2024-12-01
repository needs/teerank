import { getQueueGameTypeCount, removeAllSchedulers } from "@teerank/teerank";
import { minutesToMilliseconds } from "date-fns";
import { prisma } from "../prisma";

let maxCreatedAt = new Date(0);

export async function gameTypeScheduler() {
  const queue = getQueueGameTypeCount();

  await removeAllSchedulers(queue);
  await queue.drain();

  const schedule = async () => {
    for (; ;) {
      const gameTypes = await prisma.gameType.findMany({
        where: {
          createdAt: {
            gt: maxCreatedAt,
          },
        },
        orderBy: {
          createdAt: 'asc',
        },
        take: 50,
      });

      await Promise.all(
        gameTypes.map((gameType) =>
          queue.upsertJobScheduler(
            gameType.name,
            {
              every: minutesToMilliseconds(10),
              immediately: true,
            },
            {
              data: {
                gameTypeName: gameType.name,
              },
              opts: {
                removeOnComplete: 1000,
                removeOnFail: 1000,
              }
            }
          )
        )
      );
      console.log(`Scheduled ${gameTypes.length} new game types`);

      if (gameTypes.length > 0) {
        maxCreatedAt = gameTypes[gameTypes.length - 1].createdAt;
      } else {
        break;
      }
    }
  }

  await schedule();
  setInterval(schedule, minutesToMilliseconds(1));
}
