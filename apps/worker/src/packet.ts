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

export function unpackBytes(packet: Packet, length: number): Buffer {
  const bytes = packet.data.subarray(packet.offset, packet.offset + length);
  packet.offset += length;
  return bytes;
}

export function unpackString(packet: Packet): string {
  const startingOffset = packet.offset;

  for (; packet.offset < packet.data.length; packet.offset += 1) {
    if (packet.data[packet.offset] === 0) {
      const data = packet.data.subarray(startingOffset, packet.offset).toString();
      packet.offset += 1;
      return data;
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

// 0.7 packed int format: ESDDDDDD EDDDDDDD EDD... (E: extend, S: sign, D: data)
export function unpackInt7(packet: Packet): number {
  const masks = [0x7f, 0x7f, 0x7f, 0x0f];
  const shifts = [6, 13, 20, 27];

  const first = packet.data[packet.offset];
  const sign = (first >> 6) & 1;
  let value = first & 0x3f;

  for (let i = 0; packet.data[packet.offset] & 0x80 && i < masks.length; i += 1) {
    packet.offset += 1;
    value |= (packet.data[packet.offset] & masks[i]) << shifts[i];
  }

  packet.offset += 1;
  return sign ? ~value : value;
}

export enum ServerHeader {
  Vanilla,
  Legacy64,
  Extended,
  ExtendedMore,
  Vanilla7,
}

export function headerBuffer(header: string): Buffer {
  const headerBytes = header.split('').map((char) => char.charCodeAt(0));
  return Buffer.from([0xff, 0xff, 0xff, 0xff, ...headerBytes]);
}

const SERVER_HEADER_VANILLA = headerBuffer('inf3');
const SERVER_HEADER_LEGACY64 = headerBuffer('dtsf');
const SERVER_HEADER_EXTENDED = headerBuffer('iext');
const SERVER_HEADER_EXTENDED_MORE = headerBuffer('iex+');

// 0.6 connless packets start with 6 padding bytes, 0.7 ones with a 9 bytes
// header: flags/version byte then two 4 bytes tokens.  Returns undefined for
// anything else, like 0.7 control packets.
function unpackConnlessHeader(packet: Packet): { magic: Buffer, version7: boolean } | undefined {
  if (packet.data[0] === 0xff) {
    unpackBytes(packet, 6);
    return { magic: unpackBytes(packet, 8), version7: false };
  }

  if ((packet.data[0] & 0xfc) >> 2 === 0x08) {
    unpackBytes(packet, 9);
    return { magic: unpackBytes(packet, 8), version7: true };
  }

  return undefined;
}

export function unpackServerHeader(packet: Packet): ServerHeader | undefined {
  const connlessHeader = unpackConnlessHeader(packet);

  if (connlessHeader === undefined) {
    return undefined;
  }

  const { magic, version7 } = connlessHeader;

  if (magic.equals(SERVER_HEADER_VANILLA)) {
    return version7 ? ServerHeader.Vanilla7 : ServerHeader.Vanilla;
  } else if (magic.equals(SERVER_HEADER_LEGACY64)) {
    return ServerHeader.Legacy64;
  } else if (magic.equals(SERVER_HEADER_EXTENDED)) {
    return ServerHeader.Extended;
  } else if (magic.equals(SERVER_HEADER_EXTENDED_MORE)) {
    return ServerHeader.ExtendedMore;
  }

  throw new Error('Invalid server header');
}

export enum MasterHeader {
  Vanilla,
}

const MASTER_HEADER_VANILLA = headerBuffer('lis2');

export function unpackMasterHeader(packet: Packet): MasterHeader | undefined {
  const connlessHeader = unpackConnlessHeader(packet);

  if (connlessHeader === undefined) {
    return undefined;
  }

  if (connlessHeader.magic.equals(MASTER_HEADER_VANILLA)) {
    return MasterHeader.Vanilla;
  }

  throw new Error('Invalid master header');
}

const CTRLMSG_TOKEN = 0x05;

export function randomToken(): number {
  return Math.floor(Math.random() * 0xfffffffe);
}

// 0.7 requires a token handshake before answering connless requests: send a
// control message with our token, padded to 512 bytes, and the server replies
// with the token to use in packConnless7().
export function packCtrlTokenRequest(myToken: number): Buffer {
  const buffer = Buffer.alloc(7 + 1 + 512);
  buffer[0] = 0x04;
  buffer.writeUInt32BE(0xffffffff, 3);
  buffer[7] = CTRLMSG_TOKEN;
  buffer.writeUInt32BE(myToken, 8);
  return buffer;
}

export function peekCtrlToken(packet: Packet): number | undefined {
  const { data } = packet;

  if ((data[0] & 0xfc) >> 2 !== 0x01 || data.length < 12 || data[7] !== CTRLMSG_TOKEN) {
    return undefined;
  }

  return data.readUInt32BE(8);
}

export function packConnless7(serverToken: number, myToken: number, payload: Buffer): Buffer {
  const header = Buffer.alloc(9);
  header[0] = 0x21;
  header.writeUInt32BE(serverToken, 1);
  header.writeUInt32BE(myToken, 5);
  return Buffer.concat([header, payload]);
}
