import { prisma } from "../prisma";
import { lookup } from "dns/promises";
import { unpackMasterPackets } from "../packets/masterServerInfo";
import { resetPackets, getReceivedPackets, sendData, setupSockets } from "../socket";
import { wait } from "../utils";
import { subMinutes } from "date-fns";

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

export async function pollMasterServers() {
  const masterServerCandidate = await prisma.masterServer.findFirst({
    where: {
      OR: [{
        polledAt: {
          lt: subMinutes(new Date(), 10)
        }
      }, {
        polledAt: null,
      }],
      pollingStartedAt: null,
    },
    select: {
      id: true
    }
  });

  if (masterServerCandidate === null) {
    return false;
  }

  const masterServer = await prisma.masterServer.update({
    where: {
      id: masterServerCandidate.id,
      pollingStartedAt: null,
    },
    data: {
      pollingStartedAt: new Date(),
    },
  }).catch(() => null);

  if (masterServer === null) {
    return true;
  }

  const sockets = await setupSockets;

  console.log(`Polling ${masterServer.address}:${masterServer.port}`);
  const ip = await lookup(masterServer.address);

  sendData(sockets, PACKET_GETLIST, ip.address, masterServer.port);

  wait(2000).then(async () => {
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
          gameServers: {
            set: ids,
          },
        },
      });

      console.log(`Added ${masterServerInfo.gameServers.length} game servers (${masterServer.address}:${masterServer.port})`)
    }

    await prisma.masterServer.update({
      where: {
        id: masterServer.id,
      },
      data: {
        polledAt: new Date(),
        pollingStartedAt: null,
      },
    });

    resetPackets(sockets, ip.address, masterServer.port);
  });

  return true;
}
