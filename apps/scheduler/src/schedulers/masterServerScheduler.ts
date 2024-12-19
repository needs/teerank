import { prisma } from '../prisma';
import { minutesToMilliseconds } from 'date-fns';
import { getQueuePollMasterServer } from '@teerank/teerank';
import { schedule } from '../utils';

export async function masterServerScheduler() {
  const queue = getQueuePollMasterServer();

  schedule(minutesToMilliseconds(10), async () => {
    const masterServers = await prisma.masterServer.findMany();

    for (const masterServer of masterServers) {
      await queue.add(
        `${masterServer.address}:${masterServer.port}`,
        {
          address: masterServer.address,
          port: masterServer.port,
        },
        {
          deduplication: {
            id: `${masterServer.address}:${masterServer.port}`,
          }
        }
      )
    }

    console.log(`Scheduled ${masterServers.length} master servers`);
  });
}
