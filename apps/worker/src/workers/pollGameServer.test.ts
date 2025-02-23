import { prismaMock } from "../../test/mockPrisma";
import { ServerHeader } from "../packet";
import { GameServerInfoPacket } from "../packets/gameServerInfo";
import { GameServer, GameServerSnapshot, GameServerState, Player } from "@prisma/client";
import { processGameServerInfo } from "./pollGameServer";

const newGameServer = (): GameServer & { gameServerState: GameServerState | null } => ({
  id: 1,
  ip: 'localhost',
  port: 8303,
  lastSeenAt: new Date(),
  failureCount: 0,
  playTime: BigInt(0),
  createdAt: new Date(),
  updatedAt: new Date(),
  masterServerId: null,
  gameServerState: null,
});

const newGameServerInfo = (clients: GameServerInfoPacket['clients']): GameServerInfoPacket => ({
  version: 'version',
  name: 'name',
  gameType: 'gameType',
  map: 'map',
  numPlayers: clients.filter((client) => client.inGame).length,
  maxPlayers: clients.length,
  numClients: clients.length,
  maxClients: clients.length,
  clients,
});

const mockMap = {
  id: 1,
  name: 'map',
  gameTypeName: 'gameType',
  createdAt: new Date(),
  playTime: BigInt(0),
  playerCount: 0,
  clanCount: 0,
  gameServerCount: 0,
};

test('processGameServerInfo', async () => {
  const clients = [
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
  ]

  const gameServer = newGameServer();
  const gameServerInfo = newGameServerInfo(clients);

  prismaMock.map.upsert.mockResolvedValue(mockMap);
  prismaMock.gameServerSnapshot.create.mockResolvedValue({
    id: 1,
  } as unknown as GameServerSnapshot);

  await processGameServerInfo(gameServer, gameServerInfo);

  expect(prismaMock.clan.createMany).toHaveBeenCalledWith({
    data: [{ name: 'clan1' }, { name: 'clan2' }],
    skipDuplicates: true
  });

  expect(prismaMock.player.upsert).toHaveBeenCalledWith(expect.objectContaining({
    where: { name: 'name1' },
    update: { clanName: 'clan1', lastSeenAt: expect.any(Date) },
    create: { name: 'name1', clanName: 'clan1', lastSeenAt: expect.any(Date) },
  }));

  expect(prismaMock.player.upsert).toHaveBeenCalledWith(expect.objectContaining({
    where: { name: 'name2' },
    update: { clanName: 'clan2', lastSeenAt: expect.any(Date) },
    create: { name: 'name2', clanName: 'clan2', lastSeenAt: expect.any(Date) },
  }));

  expect(prismaMock.gameServerSnapshot.create).toHaveBeenCalledWith(expect.objectContaining({
    data: expect.objectContaining({
      clients: {
        createMany: {
          data: [
            {
              playerName: 'name1',
              clanName: 'clan1',
              country: 1,
              score: 100,
              inGame: false,
            },
            {
              playerName: 'name2',
              clanName: 'clan2',
              country: 2,
              score: 200,
              inGame: true,
            }
          ]
        }
      }
    })
  }));
});
