import { prisma } from '../prisma';
import { minutesToMilliseconds } from 'date-fns';
import { queuePollMasterServer, removeAllSchedulers } from '@teerank/teerank';

export async function masterServerScheduler() {
  const queue = queuePollMasterServer();
  await removeAllSchedulers(queue);

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
        }
      )
    )
  );

  console.log(`Scheduled ${masterServers.length} master servers`);
}
