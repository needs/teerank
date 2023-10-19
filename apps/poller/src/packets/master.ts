import { MasterHeader, Packet, packetIsConsumed, unpackBytes, unpackMasterHeader } from "../packet";

export type MasterPacket = {
  servers: {
    ip: string;
    port: number;
  }[]
}

export function unpackMasterPacket(packet: Packet): MasterPacket {
  const header = unpackMasterHeader(packet);

  switch (header) {
    case MasterHeader.Vanilla:
      return unpackMasterVanillaContent(packet);
  }
}

const ipv4Header = Buffer.from([
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xff, 0xff
]);

function unpackMasterVanillaContent(packet: Packet): MasterPacket {
  const masterPacket: MasterPacket = {
    servers: []
  }

  while (!packetIsConsumed(packet)) {
    const bytes = unpackBytes(packet, 18);
    const header = bytes.subarray(0, ipv4Header.length);

    if (header.equals(ipv4Header)) {
      masterPacket.servers.push({
        ip: Array.from(bytes.subarray(12, 16)).join('.'),
        port: bytes[16] * 256 + bytes[17],
      });
    } else {
      masterPacket.servers.push({
        ip: Array.from(bytes.subarray(0, 16)).map(byte => byte.toString(16).padStart(2, '0')).join(':'),
        port: bytes[16] * 256 + bytes[17],
      });
    }
  }

  return masterPacket;
}
