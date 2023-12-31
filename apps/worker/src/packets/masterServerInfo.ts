import {
  MasterHeader,
  Packet,
  packetIsConsumed,
  unpackBytes,
  unpackMasterHeader,
} from '../packet';

export type MasterServerPacketInfo = {
  gameServers: {
    ip: string;
    port: number;
  }[];
};

function unpackMasterPacket(packet: Packet): MasterServerPacketInfo {
  const header = unpackMasterHeader(packet);

  switch (header) {
    case MasterHeader.Vanilla:
      return unpackMasterVanillaContent(packet);
  }
}

const ipv4Header = Buffer.from([
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
]);

function unpackMasterVanillaContent(packet: Packet): MasterServerPacketInfo {
  const masterPacket: MasterServerPacketInfo = {
    gameServers: [],
  };

  while (!packetIsConsumed(packet)) {
    const bytes = unpackBytes(packet, 18);
    const header = bytes.subarray(0, ipv4Header.length);

    if (header.equals(ipv4Header)) {
      masterPacket.gameServers.push({
        ip: Array.from(bytes.subarray(12, 16)).join('.'),
        port: bytes[16] * 256 + bytes[17],
      });
    } else {
      masterPacket.gameServers.push({
        ip: Array.from(bytes.subarray(0, 16))
          .map((byte) => byte.toString(16).padStart(2, '0'))
          .reduce((acc, byte, index, array) => {
            if (index % 2 === 1) {
              return [...acc, `${array[index - 1]}${byte}`];
            } else {
              return acc;
            }
          }, [] as string[]).join(":"),
        port: bytes[16] * 256 + bytes[17],
      });
    }
  }

  return masterPacket;
}

export function unpackMasterPackets(packets: Packet[]): MasterServerPacketInfo {
  return {
    gameServers: packets.flatMap((packet) => unpackMasterPacket(packet).gameServers)
  };
}
