import { getQueueMapCount, removeAllSchedulers } from "@teerank/teerank";
import { hoursToMilliseconds, minutesToMilliseconds } from "date-fns";
import { prisma } from "../prisma";

let maxCreatedAt = new Date(0);

export async function mapScheduler() {
  const queue = getQueueMapCount();

  await removeAllSchedulers(queue);
  await queue.drain();

  const schedule = async () => {
    const maps = await prisma.map.findMany({
      where: {
        createdAt: {
          gt: maxCreatedAt,
        },
      },
      orderBy: {
        createdAt: 'asc',
      },
    });

    await Promise.all(
      maps.map((map) =>
        queue.upsertJobScheduler(
          `${map.gameTypeName} - ${map.name}`,
          {
            every: minutesToMilliseconds(10),
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
    }
  }

  schedule();
  setInterval(schedule, hoursToMilliseconds(1));
}
