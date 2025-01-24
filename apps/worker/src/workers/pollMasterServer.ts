import { prisma } from "../prisma";
import { lookup } from "dns/promises";
import { unpackMasterPackets } from "../packets/masterServerInfo";
import { resetPackets, getReceivedPackets, sendData, setupSockets, listenForPackets } from "../socket";
import { QUEUE_NAME_POLL_MASTER_SERVER, wait } from "@teerank/teerank";
import { Job, Worker } from "bullmq";
import { bullmqConnection } from "@teerank/teerank";
import { minutesToSeconds } from "date-fns";

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

    const ids = [];
    for (const { ip, port } of masterServerInfo.gameServers) {
      const result = await prisma.gameServer.upsert({
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
      });
      ids.push(result);
    }

    await prisma.masterServer.update({
      where: {
        id: masterServer.id,
      },
      data: {
        gameServers: {
          set: ids,
        },
      },
    });

    console.log(`Added ${masterServerInfo.gameServers.length} game servers (${masterServer.address}:${masterServer.port})`)
  }

  resetPackets(sockets, ip.address, masterServer.port);
}

export async function startPollMasterServerWorker() {
  return new Worker(QUEUE_NAME_POLL_MASTER_SERVER, processor, {
    connection: bullmqConnection,
    removeOnComplete: {
      age: minutesToSeconds(10),
    },
    removeOnFail: {
      count: 1000,
    }
  });
}
