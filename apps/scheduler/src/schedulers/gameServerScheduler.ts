import { getQueuePollGameServer, getQueueRankPlayer, getQueueUpdatePlayTime } from "@teerank/teerank";
import { minutesToMilliseconds } from "date-fns";
import { prisma } from "../prisma";
import { schedule, scheduleWithSpread } from "../utils";
import { captureMessage } from "@sentry/node";

let maxCreatedAt = new Date(0);
let queuesFull = false;

export async function gameServerScheduler() {
  const queue = getQueuePollGameServer();
  const queueRankPlayer = getQueueRankPlayer();
  const queueUpdatePlayTime = getQueueUpdatePlayTime();

  schedule(minutesToMilliseconds(1), async () => {
    const rankPlayerWaitingCount = await queueRankPlayer.getWaitingCount();
    const updatePlayTimeWaitingCount = await queueUpdatePlayTime.getWaitingCount();

    queuesFull = rankPlayerWaitingCount >= 50000 || updatePlayTimeWaitingCount >= 50000;
  });

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
        if (queuesFull) {
          console.log('Queues are full, skipping game server poll');
          captureMessage('Queues are full, skipping game server poll');
          return;
        }

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
