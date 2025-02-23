import {
  schedulePollGameServer,
  getUpdatePlayTimeWaitingCount,
  getRankPlayerWaitingCount
} from "@teerank/teerank";
import { minutesToMilliseconds } from "date-fns";
import { prisma } from "../prisma";
import { schedule, scheduleWithSpread } from "../utils";
import { captureMessage } from "@sentry/node";

let lastId = 0;
let queuesFull = false;

export async function gameServerScheduler() {
  schedule(minutesToMilliseconds(1), async () => {
    const [rankPlayerWaitingCount, updatePlayTimeWaitingCount] = await Promise.all([
      getRankPlayerWaitingCount(),
      getUpdatePlayTimeWaitingCount()
    ]);

    queuesFull = rankPlayerWaitingCount >= 50000 || updatePlayTimeWaitingCount >= 50000;

    if (queuesFull) {
      console.log('Queues are full:', { rankPlayerWaitingCount, updatePlayTimeWaitingCount });
    }
  });

  schedule(minutesToMilliseconds(5), async () => {
    const gameServers = await prisma.gameServer.findMany({
      where: {
        id: {
          gt: lastId,
        },
      },
      select: {
        id: true,
        ip: true,
        port: true,
      },
      orderBy: {
        id: 'asc',
      },
    });

    for (const gameServer of gameServers) {
      scheduleWithSpread(minutesToMilliseconds(5), async () => {
        if (queuesFull) {
          console.log('Queues are full, skipping game server poll');
          captureMessage('Queues are full, skipping game server poll');
          return;
        }

        await schedulePollGameServer({
          ip: gameServer.ip,
          port: gameServer.port,
          gameServerId: gameServer.id,
        });
      });
    }

    console.log(`Scheduled ${gameServers.length} new game servers`);

    if (gameServers.length > 0) {
      lastId = gameServers[gameServers.length - 1].id;
    }
  });
}
