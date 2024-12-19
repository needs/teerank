import { getQueuePollGameServer } from "@teerank/teerank";
import { minutesToMilliseconds } from "date-fns";
import { prisma } from "../prisma";
import { schedule, scheduleWithSpread } from "../utils";

let maxCreatedAt = new Date(0);

export async function gameServerScheduler() {
  const queue = getQueuePollGameServer();

  schedule(minutesToMilliseconds(5), async () => {
    const gameServers = await prisma.gameServer.findMany({
      where: {
        createdAt: {
          gt: maxCreatedAt,
        },
      },
      select: {
        ip: true,
        port: true,
        createdAt: true,
      },
      orderBy: {
        createdAt: 'asc',
      },
    });

    for (const gameServer of gameServers) {
      scheduleWithSpread(minutesToMilliseconds(5), async () => {
        await queue.add(
          `${gameServer.ip} - ${gameServer.port}`,
          {
            ip: gameServer.ip,
            port: gameServer.port,
          },
          {
            deduplication: {
              id: `${gameServer.ip} - ${gameServer.port}`,
            }
          }
        )
      });
    }

    console.log(`Scheduled ${gameServers.length} new game servers`);

    if (gameServers.length > 0) {
      maxCreatedAt = gameServers[gameServers.length - 1].createdAt;
    }
  });
}
