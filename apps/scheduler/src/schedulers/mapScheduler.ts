import { getQueueMapCount, removeAllSchedulers } from "@teerank/teerank";
import { hoursToMilliseconds, minutesToMilliseconds } from "date-fns";
import { prisma } from "../prisma";

let maxCreatedAt = new Date(0);

export async function mapScheduler() {
  const queue = getQueueMapCount();

  await removeAllSchedulers(queue);
  await queue.drain(true);

  const schedule = async () => {
    for (; ;) {
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
        take: 50,
      });

      await Promise.all(
        maps.map((map) =>
          queue.upsertJobScheduler(
            `${map.gameTypeName} - ${map.name}`,
            {
              every: hoursToMilliseconds(1),
              immediately: true,
            },
            {
              data: {
                gameTypeName: map.gameTypeName,
                mapName: map.name,
              },
              opts: {
                removeOnComplete: 1000,
                removeOnFail: 1000,
              }
            }
          )
        )
      );
      console.log(`Scheduled ${maps.length} new maps`);

      if (maps.length > 0) {
        maxCreatedAt = maps[maps.length - 1].createdAt;
      } else {
        break;
      }
    }
  }

  await schedule();
  setInterval(schedule, minutesToMilliseconds(1));
}
