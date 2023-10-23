import { prisma } from "./prisma";
import { lookup } from "dns/promises";
import { unpackMasterPacket } from "./packets/master";
import { destroySockets, getReceivedPackets, sendData, setupSockets } from "./socket";

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

    await new Promise((resolve) => setTimeout(resolve, 2000));

    const receivedPackets = getReceivedPackets(sockets, ip.address, masterServer.port);

    if (receivedPackets !== undefined) {
      for (const packet of receivedPackets.packets) {
        const masterPacket = unpackMasterPacket(packet);

        await prisma.gameServer.createMany({
          data: masterPacket.servers.map((server) => ({
            ip: server.ip,
            port: server.port,
            masterServerId: masterServer.id,
          })),
          skipDuplicates: true,
        });
      }
    }
  }

  destroySockets(sockets);
}
