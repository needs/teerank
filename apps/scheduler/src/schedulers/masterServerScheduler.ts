import { prisma } from '../prisma';
import { minutesToMilliseconds } from 'date-fns';
import { getQueuePollMasterServer, removeAllSchedulers } from '@teerank/teerank';

export async function masterServerScheduler() {
  const queue = getQueuePollMasterServer();
  await removeAllSchedulers(queue);
  await queue.drain(true);

  const masterServers = await prisma.masterServer.findMany();

  await Promise.all(
    masterServers.map((masterServer) =>
      queue.upsertJobScheduler(
        `${masterServer.address}:${masterServer.port}`,
        {
          every: minutesToMilliseconds(5),
          immediately: true,
        },
        {
          data: {
            address: masterServer.address,
            port: masterServer.port,
          },
          opts: {
            removeOnComplete: 1000,
            removeOnFail: 1000,
          }
        }
      )
    )
  );

  console.log(`Scheduled ${masterServers.length} master servers`);
}
