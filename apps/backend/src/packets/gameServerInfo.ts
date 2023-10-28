import { Packet, ServerHeader, packetIsConsumed, unpackBool, unpackInt, unpackServerHeader, unpackString } from "../packet";

export type GameServerInfoPacket = {
  version: string;
  name: string;
  gameType: string;
  map: string;

  numPlayers: number;
  maxPlayers: number;

  numClients: number;
  maxClients: number;

  clients: {
    name: string;
    clan: string;
    country: number;
    score: number;
    inGame: boolean;
  }[];
}

function initGameServerInfoPacket(): GameServerInfoPacket {
  return {
    version: '',
    name: '',
    gameType: '',
    map: '',

    numPlayers: 0,
    maxPlayers: 0,

    numClients: 0,
    maxClients: 0,

    clients: [],
  };
}

function unpackGameServerInfoPacket(packet: Packet, gameServerInfoPacket: GameServerInfoPacket) {
  const header = unpackServerHeader(packet);

  switch (header) {
    case ServerHeader.Vanilla:
      return unpackGameServerInfoVanilla(packet, gameServerInfoPacket);
    case ServerHeader.Legacy64:
      return unpackGameServerInfoLegacy64(packet, gameServerInfoPacket);
    case ServerHeader.Extended:
      return unpackGameServerInfoExtended(packet, gameServerInfoPacket);
    case ServerHeader.ExtendedMore:
      return unpackGameServerInfoExtendedMore(packet, gameServerInfoPacket);
  }
}

function unpackGameServerInfoVanilla(packet: Packet, gameServerInfoPacket: GameServerInfoPacket) {
  unpackString(packet); // token

  gameServerInfoPacket.version = unpackString(packet);
  gameServerInfoPacket.name = unpackString(packet);
  gameServerInfoPacket.map = unpackString(packet);
  gameServerInfoPacket.gameType = unpackString(packet);

  unpackString(packet); // flags

  gameServerInfoPacket.numPlayers = unpackInt(packet);
  gameServerInfoPacket.maxPlayers = unpackInt(packet);

  gameServerInfoPacket.numClients = unpackInt(packet);
  gameServerInfoPacket.maxClients = unpackInt(packet);

  while (!packetIsConsumed(packet)) {
    const name = unpackString(packet);
    const clan = unpackString(packet);
    const country = unpackInt(packet);
    const score = unpackInt(packet);
    const inGame = unpackBool(packet);

    gameServerInfoPacket.clients.push({
      name,
      clan,
      country,
      score,
      inGame,
    });
  }
}

function unpackGameServerInfoLegacy64(packet: Packet, gameServerInfoPacket: GameServerInfoPacket) {
  unpackString(packet); // token

  gameServerInfoPacket.version = unpackString(packet);
  gameServerInfoPacket.name = unpackString(packet);
  gameServerInfoPacket.map = unpackString(packet);
  gameServerInfoPacket.gameType = unpackString(packet);

  unpackString(packet); // flags

  gameServerInfoPacket.numPlayers = unpackInt(packet);
  gameServerInfoPacket.maxPlayers = unpackInt(packet);

  gameServerInfoPacket.numClients = unpackInt(packet);
  gameServerInfoPacket.maxClients = unpackInt(packet);

  unpackString(packet); // offset

  while (!packetIsConsumed(packet)) {
    const name = unpackString(packet);
    const clan = unpackString(packet);
    const country = unpackInt(packet);
    const score = unpackInt(packet);
    const inGame = unpackBool(packet);

    gameServerInfoPacket.clients.push({
      name,
      clan,
      country,
      score,
      inGame,
    });
  }
}

function unpackGameServerInfoExtended(packet: Packet, gameServerInfoPacket: GameServerInfoPacket) {
  unpackString(packet); // token

  gameServerInfoPacket.version = unpackString(packet);
  gameServerInfoPacket.name = unpackString(packet);
  gameServerInfoPacket.map = unpackString(packet);

  unpackString(packet); // map_crc
  unpackString(packet); // map_size

  gameServerInfoPacket.gameType = unpackString(packet);

  unpackString(packet); // flags

  gameServerInfoPacket.numPlayers = unpackInt(packet);
  gameServerInfoPacket.maxPlayers = unpackInt(packet);

  gameServerInfoPacket.numClients = unpackInt(packet);
  gameServerInfoPacket.maxClients = unpackInt(packet);

  unpackString(packet); // reserved

  while (!packetIsConsumed(packet)) {
    const name = unpackString(packet);
    const clan = unpackString(packet);
    const country = unpackInt(packet);
    const score = unpackInt(packet);
    const inGame = unpackBool(packet);
    unpackString(packet); // reserved

    gameServerInfoPacket.clients.push({
      name,
      clan,
      country,
      score,
      inGame,
    });
  }
}

function unpackGameServerInfoExtendedMore(packet: Packet, gameServerInfoPacket: GameServerInfoPacket) {
  unpackString(packet); // token
  unpackString(packet); // pckno

  while (!packetIsConsumed(packet)) {
    const name = unpackString(packet);
    const clan = unpackString(packet);
    const country = unpackInt(packet);
    const score = unpackInt(packet);
    const inGame = unpackBool(packet);
    unpackString(packet); // reserved

    gameServerInfoPacket.clients.push({
      name,
      clan,
      country,
      score,
      inGame,
    });
  }
}

export function unpackGameServerInfoPackets(packets: Packet[]): GameServerInfoPacket {
  const gameServerInfoPacket = initGameServerInfoPacket();

  for (const packet of packets) {
    unpackGameServerInfoPacket(packet, gameServerInfoPacket);
  }

  return gameServerInfoPacket;
}
