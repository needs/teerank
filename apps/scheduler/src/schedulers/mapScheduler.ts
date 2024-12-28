import { getQueueMapCount } from "@teerank/teerank";
import { hoursToMilliseconds, minutesToMilliseconds } from "date-fns";
import { prisma } from "../prisma";
import { schedule, scheduleWithSpread } from "../utils";

let maxCreatedAt = new Date(0);

export async function mapScheduler() {
  const queue = getQueueMapCount();

  schedule(minutesToMilliseconds(5), async () => {
    const maps = await prisma.map.findMany({
      where: {
        createdAt: {
          gt: maxCreatedAt,
        },
      },
      select: {
        gameTypeName: true,
        name: true,
        createdAt: true,
      },
      orderBy: {
        createdAt: 'asc',
      },
    });

    for (const map of maps) {
      scheduleWithSpread(hoursToMilliseconds(24), async () => {
        await queue.add(
          `${map.gameTypeName} - ${map.name}`,
          {
            gameTypeName: map.gameTypeName,
            mapName: map.name,
          },
          {
            deduplication: {
              id: `${map.gameTypeName} - ${map.name}`,
            }
          }
        );
      });
    }

    console.log(`Scheduled ${maps.length} new maps`);

    if (maps.length > 0) {
      maxCreatedAt = maps[maps.length - 1].createdAt;
    }
  });
}
