export type Packet = {
  data: Buffer;
  offset: number;
};

export function packetFromBuffer(buffer: Buffer): Packet {
  return {
    data: buffer,
    offset: 0,
  };
}

export function packetIsConsumed(packet: Packet): boolean {
  return packet.offset >= packet.data.length;
}

function unpackByte(packet: Packet): number {
  const byte = packet.data[packet.offset];
  packet.offset += 1;
  return byte;
}

export function unpackBytes(packet: Packet, length: number): Buffer {
  const bytes = packet.data.subarray(packet.offset, packet.offset + length);
  packet.offset += length;
  return bytes;
}

export function unpackString(packet: Packet): string {
  for (let offset = packet.offset; offset < packet.data.length; offset += 1) {
    if (packet.data[offset] === 0) {
      return packet.data.subarray(packet.offset, offset).toString();
    }
  }

  throw new Error('Invalid string');
}

export function unpackInt(packet: Packet): number {
  return parseInt(unpackString(packet), 10);
}

export function unpackBool(packet: Packet): boolean {
  return parseInt(unpackString(packet), 10) !== 0;
}

export enum ServerHeader {
  Vanilla,
  Legacy64,
  Extended,
  ExtendedMore,
}

export function headerBuffer(header: string): Buffer {
  const headerBytes = header.split('').map((char) => char.charCodeAt(0));
  return Buffer.from([0xff, 0xff, 0xff, 0xff, ...headerBytes]);
}

const SERVER_HEADER_VANILLA = headerBuffer('inf3');
const SERVER_HEADER_LEGACY64 = headerBuffer('dtsf');
const SERVER_HEADER_EXTENDED = headerBuffer('iext');
const SERVER_HEADER_EXTENDED_MORE = headerBuffer('iex+');

export function unpackServerHeader(packet: Packet): ServerHeader {
  unpackBytes(packet, 6);
  const header = unpackBytes(packet, 8);

  if (header.equals(SERVER_HEADER_VANILLA)) {
    return ServerHeader.Vanilla;
  } else if (header.equals(SERVER_HEADER_LEGACY64)) {
    return ServerHeader.Legacy64;
  } else if (header.equals(SERVER_HEADER_EXTENDED)) {
    return ServerHeader.Extended;
  } else if (header.equals(SERVER_HEADER_EXTENDED_MORE)) {
    return ServerHeader.ExtendedMore;
  }

  throw new Error('Invalid server header');
}

export enum MasterHeader {
  Vanilla,
}

const MASTER_HEADER_VANILLA = headerBuffer('lis2');

export function unpackMasterHeader(packet: Packet): MasterHeader {
  unpackBytes(packet, 6);
  const header = unpackBytes(packet, 8);

  if (header.equals(MASTER_HEADER_VANILLA)) {
    return MasterHeader.Vanilla;
  }

  throw new Error('Invalid master header');
}
