import { getQueuePollGameServer, removeAllSchedulers } from "@teerank/teerank";
import { minutesToMilliseconds } from "date-fns";
import { prisma } from "../prisma";

let maxCreatedAt = new Date(0);

export async function gameServerScheduler() {
  const queue = getQueuePollGameServer();

  await removeAllSchedulers(queue);
  await queue.drain();

  const schedule = async () => {
    const gameServers = await prisma.gameServer.findMany({
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
      gameServers.map((gameServer) =>
        queue.upsertJobScheduler(
          `${gameServer.ip} - ${gameServer.port}`,
          {
            every: minutesToMilliseconds(5),
            immediately: true,
          },
          {
            data: {
              ip: gameServer.ip,
              port: gameServer.port,
            },
            opts: {
              removeOnComplete: 1000,
              removeOnFail: 10000,
            }
          }
        )
      )
    );

    console.log(`Scheduled ${gameServers.length} new game servers`);

    if (gameServers.length > 0) {
      maxCreatedAt = gameServers[gameServers.length - 1].createdAt;
    }
  }

  schedule();
  setInterval(schedule, minutesToMilliseconds(1));
}
