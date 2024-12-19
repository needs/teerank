import { getQueueGameTypeCount } from "@teerank/teerank";
import { minutesToMilliseconds } from "date-fns";
import { prisma } from "../prisma";
import { schedule, scheduleWithSpread } from "../utils";

let maxCreatedAt = new Date(0);

export async function gameTypeScheduler() {
  const queue = getQueueGameTypeCount();

  schedule(minutesToMilliseconds(5), async () => {
    const gameTypes = await prisma.gameType.findMany({
      where: {
        createdAt: {
          gt: maxCreatedAt,
        },
      },
      orderBy: {
        createdAt: 'asc',
      },
    });

    for (const gameType of gameTypes) {
      scheduleWithSpread(minutesToMilliseconds(30), async () => {
        await queue.add(
          gameType.name,
          {
            gameTypeName: gameType.name,
          },
          {
            deduplication: {
              id: gameType.name,
            }
          }
        )
      });
    }

    console.log(`Scheduled ${gameTypes.length} new game types`);

    if (gameTypes.length > 0) {
      maxCreatedAt = gameTypes[gameTypes.length - 1].createdAt;
    }
  });
}
