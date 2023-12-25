import { RemoteInfo, Socket, createSocket } from "dgram";
import { Packet, packetFromBuffer } from "./packet";
import { isIP } from "net";

type Sockets = {
  socket4: Socket;
  socket6: Socket;
  packetsByIpAndPort: Record<
    string,
    { packets: Packet[]; remoteInfo: RemoteInfo }
  >;
};

function ipAndPortToString(ip: string, port: number) {
  return `${ip} | ${port}`;
}

export const setupSockets = new Promise<Sockets>((resolve) => {
  const sockets: Sockets = {
    socket4: createSocket({ type: "udp4" }),
    socket6: createSocket({ type: "udp6" }),
    packetsByIpAndPort: {},
  };

  const handleMessages = (message: Buffer, remoteInfo: RemoteInfo) => {
    const ipAndPort = ipAndPortToString(remoteInfo.address, remoteInfo.port);
    const packet = packetFromBuffer(message);

    const receivedPacket = sockets.packetsByIpAndPort[ipAndPort];

    if (receivedPacket === undefined) {
      sockets.packetsByIpAndPort[ipAndPort] = { packets: [packet], remoteInfo };
    } else {
      receivedPacket.packets.push(packet);
    }
  };

  sockets.socket4.on('message', handleMessages);
  sockets.socket6.on('message', handleMessages);

  resolve(sockets);
});

function getSocket(sockets: Sockets, ip: string) {
  switch (isIP(ip)) {
    case 4:
      return sockets.socket4;
    case 6:
      return sockets.socket6;
    default:
      throw new Error(`Invalid IP: ${ip}`);
  }
}

export function sendData(sockets: Sockets, data: Buffer, ip: string, port: number) {
  const socket = getSocket(sockets, ip);

  socket.send(data, port, ip, (err) => {
    if (err) {
      console.error(err);
    }
  });
}

export function getReceivedPackets(sockets: Sockets, ip: string, port: number) {
  const ipAndPort = ipAndPortToString(ip, port);
  return sockets.packetsByIpAndPort[ipAndPort];
}

export function resetPackets(sockets: Sockets, ip: string, port: number) {
  const ipAndPort = ipAndPortToString(ip, port);
  delete sockets.packetsByIpAndPort[ipAndPort];
}
