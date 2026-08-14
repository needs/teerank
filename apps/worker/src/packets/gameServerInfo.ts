import { Packet, ServerHeader, packetIsConsumed, unpackBool, unpackInt, unpackInt7, unpackServerHeader, unpackString } from "../packet";

type Client = {
  name: string;
  clan: string;
  country: number;
  score: number;
  inGame: boolean;

  _origin: ServerHeader;
}

export type GameServerInfoPacket = {
  version: string;
  name: string;
  gameType: string;
  map: string;

  numPlayers: number;
  maxPlayers: number;

  numClients: number;
  maxClients: number;

  clients: Client[];
}

function addClient(gameServerInfoPacket: GameServerInfoPacket, client: Client) {
  const existingClient = gameServerInfoPacket.clients.find((existingClient) => existingClient.name === client.name);

  if (existingClient !== undefined) {
    if (existingClient._origin < client._origin) {
      existingClient.clan = client.clan;
      existingClient.country = client.country;
      existingClient.score = client.score;
      existingClient.inGame = client.inGame;
      existingClient._origin = client._origin;
    }
  } else {
    gameServerInfoPacket.clients.push(client);
  }
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

function unpackGameServerInfoPacket(packet: Packet, gameServerInfoPacket: GameServerInfoPacket): boolean {
  const header = unpackServerHeader(packet);

  switch (header) {
    case undefined:
      return false;
    case ServerHeader.Vanilla:
      unpackGameServerInfoVanilla(packet, gameServerInfoPacket);
      break;
    case ServerHeader.Legacy64:
      unpackGameServerInfoLegacy64(packet, gameServerInfoPacket);
      break;
    case ServerHeader.Extended:
      unpackGameServerInfoExtended(packet, gameServerInfoPacket);
      break;
    case ServerHeader.ExtendedMore:
      unpackGameServerInfoExtendedMore(packet, gameServerInfoPacket);
      break;
    case ServerHeader.Vanilla7:
      unpackGameServerInfoVanilla7(packet, gameServerInfoPacket);
      break;
  }

  return true;
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

    addClient(gameServerInfoPacket, {
      name,
      clan,
      country,
      score,
      inGame,

      _origin: ServerHeader.Vanilla,
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

    addClient(gameServerInfoPacket, {
      name,
      clan,
      country,
      score,
      inGame,

      _origin: ServerHeader.Legacy64,
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

    addClient(gameServerInfoPacket, {
      name,
      clan,
      country,
      score,
      inGame,

      _origin: ServerHeader.Extended,
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

    addClient(gameServerInfoPacket, {
      name,
      clan,
      country,
      score,
      inGame,

      _origin: ServerHeader.ExtendedMore,
    });
  }
}

function unpackGameServerInfoVanilla7(packet: Packet, gameServerInfoPacket: GameServerInfoPacket) {
  unpackInt7(packet); // token

  gameServerInfoPacket.version = unpackString(packet);
  gameServerInfoPacket.name = unpackString(packet);

  unpackString(packet); // hostname

  gameServerInfoPacket.map = unpackString(packet);
  gameServerInfoPacket.gameType = unpackString(packet);

  unpackInt7(packet); // flags
  unpackInt7(packet); // skill level

  gameServerInfoPacket.numPlayers = unpackInt7(packet);
  gameServerInfoPacket.maxPlayers = unpackInt7(packet);

  gameServerInfoPacket.numClients = unpackInt7(packet);
  gameServerInfoPacket.maxClients = unpackInt7(packet);

  while (!packetIsConsumed(packet)) {
    const name = unpackString(packet);
    const clan = unpackString(packet);
    const country = unpackInt7(packet);
    const score = unpackInt7(packet);
    const playerType = unpackInt7(packet); // 1: spectator, 2: bot

    addClient(gameServerInfoPacket, {
      name,
      clan,
      country,
      score,
      inGame: (playerType & 1) === 0,

      _origin: ServerHeader.Vanilla7,
    });
  }
}

export function unpackGameServerInfoPackets(packets: Packet[]): GameServerInfoPacket | undefined {
  const gameServerInfoPacket = initGameServerInfoPacket();
  let unpackedCount = 0;

  for (const packet of packets) {
    if (unpackGameServerInfoPacket(packet, gameServerInfoPacket)) {
      unpackedCount += 1;
    }
  }

  return unpackedCount > 0 ? gameServerInfoPacket : undefined;
}
