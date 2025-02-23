import {
  scheduleGameTypeCount
} from "@teerank/teerank";
import { hoursToMilliseconds, minutesToMilliseconds } from "date-fns";
import { prisma } from "../prisma";
import { schedule, scheduleWithSpread } from "../utils";

let maxCreatedAt = new Date(0);

export async function gameTypeScheduler() {
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
      scheduleWithSpread(hoursToMilliseconds(1), async () => {
        await scheduleGameTypeCount({
          gameTypeName: gameType.name,
        });
      });
    }

    console.log(`Scheduled ${gameTypes.length} new game types`);

    if (gameTypes.length > 0) {
      maxCreatedAt = gameTypes[gameTypes.length - 1].createdAt;
    }
  });
}
