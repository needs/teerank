import { headerBuffer, packetFromBuffer, peekCtrlToken, unpackInt7 } from "../packet";
import { unpackGameServerInfoPackets } from "./gameServerInfo";
import { unpackMasterPackets } from "./masterServerInfo";

function packInt7(value: number): number[] {
  let sign = 0;
  if (value < 0) {
    sign = 0x40;
    value = ~value;
  }

  const bytes = [sign | (value & 0x3f)];
  value >>>= 6;

  while (value) {
    bytes[bytes.length - 1] |= 0x80;
    bytes.push(value & 0x7f);
    value >>>= 7;
  }

  return bytes;
}

function packString(value: string): number[] {
  return [...Buffer.from(value), 0];
}

function connless7Packet(magic: string, payload: number[]) {
  return packetFromBuffer(Buffer.from([
    0x21,
    0x11, 0x22, 0x33, 0x44,
    0x55, 0x66, 0x77, 0x88,
    ...headerBuffer(magic),
    ...payload,
  ]));
}

function ctrlTokenPacket(serverToken: number) {
  const buffer = Buffer.alloc(12);
  buffer[0] = 0x04;
  buffer[7] = 0x05;
  buffer.writeUInt32BE(serverToken, 8);
  return packetFromBuffer(buffer);
}

test('unpackInt7', () => {
  for (const value of [0, 1, 63, 64, -1, -64, 1234567, -1234567]) {
    const packet = packetFromBuffer(Buffer.from(packInt7(value)));
    expect(unpackInt7(packet)).toBe(value);
    expect(packet.offset).toBe(packet.data.length);
  }
});

test('peekCtrlToken', () => {
  expect(peekCtrlToken(ctrlTokenPacket(0xaabbccdd))).toBe(0xaabbccdd);
  expect(peekCtrlToken(connless7Packet('inf3', []))).toBeUndefined();
});

test('unpackGameServerInfoPackets version 0.7', () => {
  const packet = connless7Packet('inf3', [
    ...packInt7(0), // token
    ...packString('0.7.5'),
    ...packString('server name'),
    ...packString('hostname'),
    ...packString('ctf5'),
    ...packString('CTF'),
    ...packInt7(1), // flags
    ...packInt7(1), // skill level
    ...packInt7(1),
    ...packInt7(8),
    ...packInt7(2),
    ...packInt7(16),

    ...packString('player1'),
    ...packString('clan1'),
    ...packInt7(-1),
    ...packInt7(10),
    ...packInt7(0),

    ...packString('spectator1'),
    ...packString(''),
    ...packInt7(64),
    ...packInt7(-3),
    ...packInt7(1),
  ]);

  expect(unpackGameServerInfoPackets([packet])).toEqual({
    version: '0.7.5',
    name: 'server name',
    map: 'ctf5',
    gameType: 'CTF',
    numPlayers: 1,
    maxPlayers: 8,
    numClients: 2,
    maxClients: 16,
    clients: [
      expect.objectContaining({
        name: 'player1',
        clan: 'clan1',
        country: -1,
        score: 10,
        inGame: true,
      }),
      expect.objectContaining({
        name: 'spectator1',
        clan: '',
        country: 64,
        score: -3,
        inGame: false,
      }),
    ],
  });
});

test('unpackGameServerInfoPackets skips control packets', () => {
  expect(unpackGameServerInfoPackets([ctrlTokenPacket(0x12345678)])).toBeUndefined();
});

test('unpackMasterPackets version 0.7', () => {
  const listPacket = connless7Packet('lis2', [
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 1, 2, 3, 4, 0x20, 0x6f,
  ]);

  expect(unpackMasterPackets([ctrlTokenPacket(0x12345678), listPacket])).toEqual({
    gameServers: [
      {
        ip: '1.2.3.4',
        port: 8303,
      },
    ],
  });
});
