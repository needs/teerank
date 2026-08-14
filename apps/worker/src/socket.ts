import { RemoteInfo, Socket, createSocket } from "dgram";
import { Packet, packCtrlTokenRequest, packConnless7, packetFromBuffer, peekCtrlToken, randomToken } from "./packet";
import { isIP } from "net";

type Sockets = {
  socket4: Socket;
  socket6: Socket;
  packetsByIpAndPort: Record<
    string,
    { packets: Packet[]; onPacket?: (packet: Packet) => void; }
  >;
};

function ipAndPortToString(ip: string, port: number) {
  return `${ip} | ${port}`;
}

let socketsInstance: Promise<Sockets> | null = null;

async function createSockets() {
  const sockets: Sockets = {
    socket4: createSocket({ type: "udp4" }),
    socket6: createSocket({ type: "udp6" }),
    packetsByIpAndPort: {},
  };

  const handleMessages = (message: Buffer, remoteInfo: RemoteInfo) => {
    const ipAndPort = ipAndPortToString(remoteInfo.address, remoteInfo.port);
    const packet = packetFromBuffer(message);

    const receivedPacket = sockets.packetsByIpAndPort[ipAndPort];

    if (receivedPacket !== undefined) {
      receivedPacket.packets.push(packet);
      receivedPacket.onPacket?.(packet);
    }
  };

  sockets.socket4.on('message', handleMessages);
  sockets.socket6.on('message', handleMessages);

  return sockets;
}

export async function setupSockets() {
  if (!socketsInstance) {
    socketsInstance = createSockets();
  }

  return socketsInstance;
}

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

export function listenForPackets(sockets: Sockets, ip: string, port: number, onPacket?: (packet: Packet) => void) {
  const ipAndPort = ipAndPortToString(ip, port);
  sockets.packetsByIpAndPort[ipAndPort] = { packets: [], onPacket };
}

// Sends 0.6 requests right away.  0.7 requires a token handshake first, so the
// 0.7 request is sent when the server answers the token request.
export function sendRequests(sockets: Sockets, ip: string, port: number, requests06: Buffer[], request7: Buffer) {
  const myToken = randomToken();

  listenForPackets(sockets, ip, port, (packet) => {
    const serverToken = peekCtrlToken(packet);

    if (serverToken !== undefined) {
      sendData(sockets, packConnless7(serverToken, myToken, request7), ip, port);
    }
  });

  for (const request of requests06) {
    sendData(sockets, request, ip, port);
  }

  sendData(sockets, packCtrlTokenRequest(myToken), ip, port);
}

export function getReceivedPackets(sockets: Sockets, ip: string, port: number) {
  const ipAndPort = ipAndPortToString(ip, port);
  return sockets.packetsByIpAndPort[ipAndPort];
}

export function resetPackets(sockets: Sockets, ip: string, port: number) {
  const ipAndPort = ipAndPortToString(ip, port);
  delete sockets.packetsByIpAndPort[ipAndPort];
}
