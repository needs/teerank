import { clearDatabase } from "../../testSetup";
import { ServerHeader } from "../packet";
import { GameServerInfoPacket } from "../packets/gameServerInfo";
import { prisma } from "../prisma";
import { OnUpdatePlayerClanHook, processGameServerInfo } from "./pollGameServer";

beforeEach(async () => {
  await clearDatabase();
});

async function createAndProcessGameServerInfo(
  clients: GameServerInfoPacket['clients'],
  onUpdatePlayerClanHook: OnUpdatePlayerClanHook | undefined = undefined
) {
  const gameServer = await prisma.gameServer.upsert({
    where: {
      ip_port: {
        ip: 'localhost',
        port: 8303,
      },
    },
    update: {},
    create: {
      ip: 'localhost',
      port: 8303,
    },
    include: {
      gameServerState: true
    }
  });

  const gameServerInfo = {
    version: 'version',
    name: 'name',
    gameType: 'gameType',
    map: 'map',
    numPlayers: clients.filter((client) => client.inGame).length,
    maxPlayers: clients.length,
    numClients: clients.length,
    maxClients: clients.length,
    clients,
  };

  const snapshotId = await processGameServerInfo(gameServer, gameServerInfo, onUpdatePlayerClanHook);

  return { gameServer, gameServerInfo, snapshotId };
}

test('Header data', async () => {
  const { gameServer, gameServerInfo, snapshotId } = await createAndProcessGameServerInfo([]);

  const snapshot = await prisma.gameServerSnapshot.findUniqueOrThrow({
    where: {
      id: snapshotId,
    },
    include: {
      map: true,
      gameServer: {
        include: {
          gameServerState: {
            include: {
              clients: true,
            }
          }
        }
      }
    }
  });

  const gameServerState = snapshot.gameServer.gameServerState;

  expect(snapshot.gameServerId).toBe(gameServer.id);
  expect(snapshot.map.name).toBe(gameServerInfo.map);
  expect(snapshot.map.gameTypeName).toBe(gameServerInfo.gameType);
  expect(snapshot.numPlayers).toBe(gameServerInfo.numPlayers);
  expect(snapshot.maxPlayers).toBe(gameServerInfo.maxPlayers);
  expect(snapshot.numClients).toBe(gameServerInfo.numClients);
  expect(snapshot.maxClients).toBe(gameServerInfo.maxClients);
  expect(gameServerState).not.toBeNull();

  if (gameServerState !== null) {
    expect(gameServerState.version).toBe(gameServerInfo.version);
    expect(gameServerState.name).toBe(gameServerInfo.name);
    expect(gameServerState.mapId).toBe(snapshot.map.id);
    expect(gameServerState.numPlayers).toBe(gameServerInfo.numPlayers);
    expect(gameServerState.maxPlayers).toBe(gameServerInfo.maxPlayers);
    expect(gameServerState.numClients).toBe(gameServerInfo.numClients);
    expect(gameServerState.maxClients).toBe(gameServerInfo.maxClients);
    expect(gameServerState.clients.length).toBe(0);
  }
});

test('Different clients', async () => {
  const { gameServerInfo } = await createAndProcessGameServerInfo([
    {
      name: 'name1',
      clan: 'clan1',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
    {
      name: 'name2',
      clan: 'clan2',
      country: 2,
      score: 200,
      inGame: true,
      _origin: ServerHeader.Vanilla,
    },
  ]);

  const players = await prisma.player.findMany({
    orderBy: {
      name: 'asc',
    },
  });

  expect(players.length).toBe(2);
  expect(players[0].name).toBe(gameServerInfo.clients[0].name);
  expect(players[1].name).toBe(gameServerInfo.clients[1].name);

  expect(players[0].clanName).toBe(gameServerInfo.clients[0].clan);
  expect(players[1].clanName).toBe(gameServerInfo.clients[1].clan);

  const clans = await prisma.clan.findMany({
    orderBy: {
      name: 'asc',
    },
  });

  expect(clans.length).toBe(2);
  expect(clans[0].name).toBe(gameServerInfo.clients[0].clan);
  expect(clans[1].name).toBe(gameServerInfo.clients[1].clan);
});

test('Duplicated clients', async () => {
  const { gameServerInfo, snapshotId } = await createAndProcessGameServerInfo([
    {
      name: 'name1',
      clan: 'clan1',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
    {
      name: 'name1',
      clan: 'clan1',
      country: 2,
      score: 200,
      inGame: true,
      _origin: ServerHeader.Vanilla,
    },
  ]);

  const players = await prisma.player.findMany({
    orderBy: {
      name: 'asc',
    },
  });

  expect(players.length).toBe(1);
  expect(players[0].name).toBe(gameServerInfo.clients[0].name);
  expect(players[0].clanName).toBe(gameServerInfo.clients[0].clan);

  const clans = await prisma.clan.findMany({
    orderBy: {
      name: 'asc',
    },
  });

  expect(clans.length).toBe(1);
  expect(clans[0].name).toBe(gameServerInfo.clients[0].clan);

  const snapshot = await prisma.gameServerSnapshot.findFirstOrThrow({
    where: {
      id: snapshotId,
    },
    include: {
      clients: true,
    }
  });

  expect(snapshot.clients.length).toBe(2);
  expect(snapshot.clients[0].playerName).toBe(gameServerInfo.clients[0].name);
  expect(snapshot.clients[0].clanName).toBe(gameServerInfo.clients[0].clan);
  expect(snapshot.clients[1].playerName).toBe(gameServerInfo.clients[1].name);
  expect(snapshot.clients[1].clanName).toBe(gameServerInfo.clients[1].clan);
});

test('Empty clan', async () => {
  const { gameServerInfo } = await createAndProcessGameServerInfo([
    {
      name: 'name1',
      clan: '',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
  ]);

  const player = await prisma.player.findUniqueOrThrow({
    where: {
      name: gameServerInfo.clients[0].name,
    },
  });

  expect(player.clanName).toBeNull();

  const clans = await prisma.clan.findMany({
    orderBy: {
      name: 'asc',
    },
  });

  expect(clans.length).toBe(0);
});

test('Clan changes', async () => {
  await createAndProcessGameServerInfo([
    {
      name: 'name1',
      clan: 'clan1',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
  ]);

  const { gameServerInfo } = await createAndProcessGameServerInfo([
    {
      name: 'name1',
      clan: 'clan2',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
  ]);

  const player = await prisma.player.findUniqueOrThrow({
    where: {
      name: gameServerInfo.clients[0].name,
    },
  });

  expect(player.clanName).toBe(gameServerInfo.clients[0].clan);

  const clan1 = await prisma.clan.findUniqueOrThrow({
    where: {
      name: 'clan1',
    },
  });

  expect(clan1.activePlayerCount).toBe(0);

  const clan2 = await prisma.clan.findUniqueOrThrow({
    where: {
      name: 'clan2',
    },
  });

  expect(clan2.activePlayerCount).toBe(1);
});

test('Clan swap', async () => {
  await createAndProcessGameServerInfo([
    {
      name: 'name1',
      clan: 'clan1',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
    {
      name: 'name2',
      clan: 'clan2',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
  ]);

  const { gameServerInfo } = await createAndProcessGameServerInfo([
    {
      name: 'name1',
      clan: 'clan2',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
    {
      name: 'name2',
      clan: 'clan1',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
  ]);

  const player1 = await prisma.player.findUniqueOrThrow({
    where: {
      name: gameServerInfo.clients[0].name,
    },
  });

  const player2 = await prisma.player.findUniqueOrThrow({
    where: {
      name: gameServerInfo.clients[1].name,
    },
  });

  expect(player1.clanName).toBe(gameServerInfo.clients[0].clan);
  expect(player2.clanName).toBe(gameServerInfo.clients[1].clan);

  const clan1 = await prisma.clan.findUniqueOrThrow({
    where: {
      name: gameServerInfo.clients[0].clan,
    },
  });

  expect(clan1.activePlayerCount).toBe(1);

  const clan2 = await prisma.clan.findUniqueOrThrow({
    where: {
      name: gameServerInfo.clients[1].clan,
    },
  });

  expect(clan2.activePlayerCount).toBe(1);
});

test('Clan removal', async () => {
  await createAndProcessGameServerInfo([
    {
      name: 'name1',
      clan: 'clan1',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
    {
      name: 'name2',
      clan: 'clan1',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
  ]);

  const { gameServerInfo } = await createAndProcessGameServerInfo([
    {
      name: 'name1',
      clan: '',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
    {
      name: 'name2',
      clan: '',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
  ]);

  const player1 = await prisma.player.findUniqueOrThrow({
    where: {
      name: gameServerInfo.clients[0].name,
    },
  });

  const player2 = await prisma.player.findUniqueOrThrow({
    where: {
      name: gameServerInfo.clients[1].name,
    },
  });

  expect(player1.clanName).toBeNull();
  expect(player2.clanName).toBeNull();

  const clan1 = await prisma.clan.findUniqueOrThrow({
    where: {
      name: 'clan1',
    },
  });

  expect(clan1.activePlayerCount).toBe(0);
});

test('Clan changes with race condition', async () => {
  await createAndProcessGameServerInfo([
    {
      name: 'name1',
      clan: 'clan1',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
  ]);

  const { gameServerInfo } = await createAndProcessGameServerInfo([
    {
      name: 'name1',
      clan: 'clan3',
      country: 1,
      score: 100,
      inGame: false,
      _origin: ServerHeader.Vanilla,
    },
  ], async (playerName, oldClanName) => {
    if (oldClanName !== 'clan2') {
      await createAndProcessGameServerInfo([
        {
          name: playerName,
          clan: 'clan2',
          country: 1,
          score: 100,
          inGame: false,
          _origin: ServerHeader.Vanilla,
        },
      ]);
    }
  });

  const player1 = await prisma.player.findUniqueOrThrow({
    where: {
      name: gameServerInfo.clients[0].name,
    },
  });

  expect(player1.clanName).toBe('clan3');

  const clan1 = await prisma.clan.findUniqueOrThrow({
    where: {
      name: 'clan1',
    },
  });

  expect(clan1.activePlayerCount).toBe(0);

  const clan2 = await prisma.clan.findUniqueOrThrow({
    where: {
      name: 'clan2',
    },
  });

  expect(clan2.activePlayerCount).toBe(0);

  const clan3 = await prisma.clan.findUniqueOrThrow({
    where: {
      name: 'clan3',
    },
  });

  expect(clan3.activePlayerCount).toBe(1);
});
