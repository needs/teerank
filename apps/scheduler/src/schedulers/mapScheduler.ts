import { getQueueMapCount } from "@teerank/teerank";
import { hoursToMilliseconds, minutesToMilliseconds } from "date-fns";
import { prisma } from "../prisma";
import { schedule, scheduleWithSpread } from "../utils";

let lastId = 0;

export async function mapScheduler() {
  const queue = getQueueMapCount();

  schedule(minutesToMilliseconds(5), async () => {
    const maps = await prisma.map.findMany({
      where: {
        id: {
          gt: lastId,
        },
      },
      select: {
        gameTypeName: true,
        name: true,
        id: true,
      },
      orderBy: {
        id: 'asc',
      },
    });

    for (const map of maps) {
      scheduleWithSpread(hoursToMilliseconds(24), async () => {
        await queue.add(
          `${map.gameTypeName} - ${map.name}`,
          {
            gameTypeName: map.gameTypeName,
            mapName: map.name,
            mapId: map.id,
          },
          {
            deduplication: {
              id: map.id.toString(),
            }
          }
        );
      });
    }

    console.log(`Scheduled ${maps.length} new maps`);

    if (maps.length > 0) {
      lastId = maps[maps.length - 1].id;
    }
  });
}
