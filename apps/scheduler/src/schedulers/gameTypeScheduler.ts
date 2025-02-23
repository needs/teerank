import {
  scheduleGameTypeCount
} from "@teerank/teerank";
import { hoursToMilliseconds, minutesToMilliseconds } from "date-fns";
import { prisma } from "../prisma";
import { schedule, scheduleWithSpread } from "../utils";

let lastId = 0;

export async function gameTypeScheduler() {
  schedule(minutesToMilliseconds(5), async () => {
    const gameTypes = await prisma.gameType.findMany({
      where: {
        id: {
          gt: lastId,
        },
      },
      orderBy: {
        id: 'asc',
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
      lastId = gameTypes[gameTypes.length - 1].id;
    }
  });
}
