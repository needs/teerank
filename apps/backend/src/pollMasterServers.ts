import { prisma } from "./prisma";
import { lookup } from "dns/promises";
import { unpackMasterPackets } from "./packets/masterServerInfo";
import { destroySockets, getReceivedPackets, sendData, setupSockets } from "./socket";
import { wait } from "./utils";

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

      const gameServersCreated = await prisma.gameServer.createMany({
        data: masterServerInfo.gameServers.map(({ ip, port }) => ({
          ip,
          port,
          masterServerId: masterServer.id,
        })),
        skipDuplicates: true,
      });

      console.log(`Added ${gameServersCreated.count} game servers (${masterServer.address}:${masterServer.port})`)
    }
  }

  destroySockets(sockets);
}
