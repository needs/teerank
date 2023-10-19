import { PrismaClient } from '@prisma/client'
import { RemoteInfo, createSocket } from 'dgram';
import { unpackMasterPacket } from './packets/master';
import { Packet, packetFromBuffer } from './packet';
import { lookup } from 'dns/promises';

const prisma = new PrismaClient()

function ipAndPortToString(ip: string, port: number) {
  return `${ip} | ${port}`;
}

const packetsByIpAndPort: Record<string, { packets: Packet[], remoteInfo: RemoteInfo }> = {}

const socket4 = createSocket({ type: 'udp4' });

socket4.on('message', (message, remoteInfo) => {
  const ipAndPort = ipAndPortToString(remoteInfo.address, remoteInfo.port);
  const packet = packetFromBuffer(message);

  const receivedPacket = packetsByIpAndPort[ipAndPort];

  if (receivedPacket === undefined) {
    packetsByIpAndPort[ipAndPort] = { packets: [packet], remoteInfo };
  } else {
    receivedPacket.packets.push(packet);
  }
});

function stringToCharCode(str: string) {
  return str.split('').map(char => char.charCodeAt(0));
}

const PACKET_HEADER = Buffer.from([...stringToCharCode("xe"), 0xff, 0xff, 0xff, 0xff]);
const PACKET_GETLIST = Buffer.from([...PACKET_HEADER, 0xff, 0xff, 0xff, 0xff, ...stringToCharCode("req2")]);

async function pollMasterServers() {
  const masterServers = await prisma.masterServer.findMany();

  for (const masterServer of masterServers) {
    console.log(`Polling ${masterServer.address}:${masterServer.port}`)
    const ip = await lookup(masterServer.address);

    socket4.send(PACKET_GETLIST, masterServer.port, ip.address, (err) => {
      if (err) {
        console.error(err);
      }
    });

    await new Promise(resolve => setTimeout(resolve, 2000));

    const ipAndPort = ipAndPortToString(ip.address, masterServer.port);
    const receivedPacket = packetsByIpAndPort[ipAndPort];

    if (receivedPacket !== undefined) {
      for (const packet of receivedPacket.packets) {
        const masterPacket = unpackMasterPacket(packet);

        await prisma.gameServer.createMany({
          data: masterPacket.servers.map(server => ({
            ip: server.ip,
            port: server.port,
            masterServerId: masterServer.id,
          })),
          skipDuplicates: true,
        });
      }
    }
  }
}

async function main() {
  await pollMasterServers();
}

main();
