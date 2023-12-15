import { prisma } from "../prisma";
import { lookup } from "dns/promises";
import { unpackMasterPackets } from "../packets/masterServerInfo";
import { destroySockets, getReceivedPackets, sendData, setupSockets } from "../socket";
import { wait } from "../utils";
import { Task } from "../task";
import { TaskRunStatus } from "@prisma/client";

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

export async function pollMasterServers(rangeStart: number, rangeEnd: number) {
  const sockets = setupSockets();

  const masterServers = await prisma.masterServer.findMany();

  for (const masterServer of masterServers) {
    console.log(`Polling ${masterServer.address}:${masterServer.port}`);
    const ip = await lookup(masterServer.address);

    sendData(sockets, PACKET_GETLIST, ip.address, masterServer.port);

    await wait(2000);

    const receivedPackets = getReceivedPackets(sockets, ip.address, masterServer.port);

    if (receivedPackets !== undefined) {
      const masterServerInfo = unpackMasterPackets(receivedPackets.packets)

      const ids = await prisma.$transaction(
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
  }

  destroySockets(sockets);
}
