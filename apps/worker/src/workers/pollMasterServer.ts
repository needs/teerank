import { prisma } from "../prisma";
import { lookup } from "dns/promises";
import { unpackMasterPackets } from "../packets/masterServerInfo";
import { resetPackets, getReceivedPackets, sendData, setupSockets, listenForPackets } from "../socket";
import { wait } from "@teerank/teerank";
import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection } from "../bullmq";
import { minutesToMilliseconds } from "date-fns";

function stringToCharCode(str: string) {
  return str.split('').map((char) => char.charCodeAt(0));
}

const PACKET_HEADER = Buffer.from([
  ...stringToCharCode('xe'),
  0xff,
  0xff,
  0xff,
  0xff,
]);
const PACKET_GETLIST = Buffer.from([
  ...PACKET_HEADER,
  0xff,
  0xff,
  0xff,
  0xff,
  ...stringToCharCode('req2'),
]);

async function processor(job: Job) {
  const masterServer = await prisma.masterServer.findUniqueOrThrow({
    where: {
      address_port: {
        address: job.data.address,
        port: job.data.port,
      },
    },
  });

  const sockets = await setupSockets;

  console.log(`Polling ${masterServer.address}:${masterServer.port}`);
  const ip = await lookup(masterServer.address);

  listenForPackets(sockets, ip.address, masterServer.port);

  sendData(sockets, PACKET_GETLIST, ip.address, masterServer.port);

  await wait(2000);
  const receivedPackets = getReceivedPackets(sockets, ip.address, masterServer.port);

  if (receivedPackets !== undefined) {
    const masterServerInfo = unpackMasterPackets(receivedPackets.packets)

    const ids = await Promise.all(
      masterServerInfo.gameServers.map(({ ip, port }) =>
        prisma.gameServer.upsert({
          where: {
            ip_port: {
              ip,
              port,
            },
          },
          select: {
            id: true,
          },
          update: {},
          create: {
            ip,
            port,
          },
        })
      )
    );

    await prisma.masterServer.update({
      where: {
        id: masterServer.id,
      },
      data: {
        polledAt: new Date(),
        gameServers: {
          set: ids,
        },
      },
    });

    console.log(`Added ${masterServerInfo.gameServers.length} game servers (${masterServer.address}:${masterServer.port})`)
  }

  resetPackets(sockets, ip.address, masterServer.port);
}

const queue = new Queue('poll-master-server', { connection: bullmqConnection });

export async function startPollMasterServerWorker() {
  const masterServers = await prisma.masterServer.findMany();
  console.log(`Found ${masterServers.length} master servers`);

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

  new Worker('poll-master-server', processor, {
    connection: bullmqConnection,
  });
}
