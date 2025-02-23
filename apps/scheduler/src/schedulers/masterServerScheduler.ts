import { prisma } from '../prisma';
import { minutesToMilliseconds } from 'date-fns';
import { schedulePollMasterServer } from '@teerank/teerank';
import { schedule } from '../utils';

export async function masterServerScheduler() {
  schedule(minutesToMilliseconds(10), async () => {
    const masterServers = await prisma.masterServer.findMany();

    for (const masterServer of masterServers) {
      await schedulePollMasterServer({
        address: masterServer.address,
        port: masterServer.port,
      });
    }

    console.log(`Scheduled ${masterServers.length} master servers`);
  });
}
