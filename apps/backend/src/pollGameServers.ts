import { prisma } from "./prisma";
import { destroySockets, getReceivedPackets, sendData, setupSockets } from "./socket";
import { unpackGameServerInfoPackets } from "./packets/gameServerInfo";
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

const PACKET_GETINFO = Buffer.from([
  ...PACKET_HEADER,
  0xff,
  0xff,
  0xff,
  0xff,
  ...stringToCharCode('gie3'),
  0
]);

const PACKET_GETINFO64 = Buffer.from([
  ...PACKET_HEADER,
  0xff,
  0xff,
  0xff,
  0xff,
  ...stringToCharCode('fstd'),
  0
]);

export async function pollGameServers() {
  const sockets = setupSockets();

  const gameServers = await prisma.gameServer.findMany();

  await Promise.all(gameServers.map(async (gameServer, index) => {
    await wait(index * 100);

    console.log(`${gameServer.ip}:${gameServer.port}: Polling`);

    sendData(sockets, PACKET_GETINFO, gameServer.ip, gameServer.port);
    sendData(sockets, PACKET_GETINFO64, gameServer.ip, gameServer.port);

    await wait(2000);

    const receivedPackets = getReceivedPackets(sockets, gameServer.ip, gameServer.port);

    if (receivedPackets !== undefined) {
      try {
        const gameServerInfo = unpackGameServerInfoPackets(receivedPackets.packets)
        console.log(`${gameServer.ip}:${gameServer.port}: ${JSON.stringify(gameServerInfo, undefined, 2)}`)
      } catch (e) {
        console.warn(`${gameServer.ip}:${gameServer.port}: ${e.message}`)
      }
    } else {
      console.log(`${gameServer.ip}:${gameServer.port}: No response`);
    }
  }))

  destroySockets(sockets);
}
