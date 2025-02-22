import { prismaMock } from "../../test/mockPrisma";
import { ServerHeader } from "../packet";
import { GameServerInfoPacket } from "../packets/gameServerInfo";
import { GameServer, GameServerSnapshot, GameServerState, Player } from "@prisma/client";
import { changePlayerClans, processGameServerInfo } from "./pollGameServer";

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

  prismaMock.player.findUnique.mockResolvedValue({ clanName: null } as unknown as Player);
  prismaMock.player.updateMany.mockResolvedValue({ count: 1 });

  await processGameServerInfo(gameServer, gameServerInfo);

  expect(prismaMock.clan.createMany).toHaveBeenCalledWith({
    data: [{ name: 'clan1' }, { name: 'clan2' }],
    skipDuplicates: true
  });

  expect(prismaMock.player.createMany).toHaveBeenCalledWith({
    data: [{ name: 'name1' }, { name: 'name2' }],
    skipDuplicates: true
  });

  expect(prismaMock.player.updateMany).toHaveBeenCalledWith({
    where: { name: 'name1', clanName: null },
    data: { clanName: 'clan1' }
  });
  expect(prismaMock.player.updateMany).toHaveBeenCalledWith({
    where: { name: 'name2', clanName: null },
    data: { clanName: 'clan2' }
  });

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

describe('changePlayerClan', () => {

  test('Empty clan', async () => {
    prismaMock.player.findUnique.mockResolvedValue({ clanName: 'clan1' } as unknown as Player);
    prismaMock.player.updateMany.mockResolvedValue({ count: 1 });

    await changePlayerClans({
      'name1': null,
    }, undefined);

    expect(prismaMock.player.updateMany).toHaveBeenCalledWith({
      where: { name: 'name1', clanName: 'clan1' },
      data: { clanName: null }
    });

    expect(prismaMock.clan.update).toHaveBeenCalledWith({
      where: { name: 'clan1' },
      data: { activePlayerCount: { increment: -1 } }
    });
  });

  test('Clan changes', async () => {
    prismaMock.player.findUnique.mockResolvedValue({ clanName: 'clan1' } as unknown as Player);
    prismaMock.player.updateMany.mockResolvedValue({ count: 1 });

    await changePlayerClans({
      'name1': 'clan2',
    }, undefined);

    expect(prismaMock.player.updateMany).toHaveBeenCalledWith({
      where: { name: 'name1', clanName: 'clan1' },
      data: { clanName: 'clan2' }
    });

    expect(prismaMock.clan.update).toHaveBeenCalledWith({
      where: { name: 'clan1' },
      data: { activePlayerCount: { increment: -1 } }
    });

    expect(prismaMock.clan.update).toHaveBeenCalledWith({
      where: { name: 'clan2' },
      data: { activePlayerCount: { increment: 1 } }
    });
  });

  test('Clan changes with race condition', async () => {
    prismaMock.player.findUnique.mockResolvedValueOnce({ clanName: 'clan1' } as unknown as Player);
    prismaMock.player.findUnique.mockResolvedValueOnce({ clanName: 'clan1-bis' } as unknown as Player);
    prismaMock.player.updateMany.mockResolvedValueOnce({ count: 0 });
    prismaMock.player.updateMany.mockResolvedValueOnce({ count: 1 });

    await changePlayerClans({
      'name1': 'clan2',
    }, undefined);

    expect(prismaMock.player.updateMany).toHaveBeenCalledWith({
      where: { name: 'name1', clanName: 'clan1' },
      data: { clanName: 'clan2' }
    });
    expect(prismaMock.player.updateMany).toHaveBeenCalledWith({
      where: { name: 'name1', clanName: 'clan1-bis' },
      data: { clanName: 'clan2' }
    });

    expect(prismaMock.clan.update).toHaveBeenCalledWith({
      where: { name: 'clan1-bis' },
      data: { activePlayerCount: { increment: -1 } }
    });
    expect(prismaMock.clan.update).toHaveBeenCalledWith({
      where: { name: 'clan2' },
      data: { activePlayerCount: { increment: 1 } }
    });
  });
});
