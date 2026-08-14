import { prisma } from "../prisma";
import { lookup } from "dns/promises";
import { unpackMasterPackets } from "../packets/masterServerInfo";
import { resetPackets, getReceivedPackets, sendRequests, setupSockets } from "../socket";
import { PollMasterServerJobData, processPollMasterServerJobs, wait } from "@teerank/teerank";
import { headerBuffer } from "../packet";

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

const REQUEST_GETLIST7 = headerBuffer('req2');

async function processor(jobData: PollMasterServerJobData) {
  const masterServer = await prisma.masterServer.findUniqueOrThrow({
    where: {
      address_port: {
        address: jobData.address,
        port: jobData.port,
      },
    },
  });

  const sockets = await setupSockets();

  console.log(`Polling ${masterServer.address}:${masterServer.port}`);
  const ip = await lookup(masterServer.address);

  sendRequests(sockets, ip.address, masterServer.port, [PACKET_GETLIST], REQUEST_GETLIST7);

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
  return processPollMasterServerJobs(processor);
}
