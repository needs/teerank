import { addMinutes } from "date-fns";
import { prismaMock } from "../../test/mockPrisma";
import { updatePlayTime } from "./updatePlayTime";
import { GameServerSnapshot, GameServerClient } from "@prisma/client";

const newClient = (playerName: string, clanName?: string): GameServerClient => ({
  playerName,
  clanName: clanName ?? null,
  score: 0,
  inGame: true,
  country: 0,
  snapshotId: 1,
  id: 1,
});

const newSnapshot = (id: number, createdAt: Date, clients: GameServerClient[]): Partial<GameServerSnapshot> & {
  id: number,
  map: { gameTypeName: string },
  clients: GameServerClient[]
} => ({
  id,
  createdAt,
  name: 'snapshot',
  version: 'version',
  maxClients: clients.length,
  numClients: clients.length,
  maxPlayers: clients.length,
  numPlayers: clients.length,
  gameServerId: 1,
  mapId: 1,
  map: {
    gameTypeName: 'gameType'
  },
  clients
});

const mockFirstSnapshot = (baseDate: Date, clients: GameServerClient[]) => {
  const snapshot = newSnapshot(1, baseDate, clients);
  prismaMock.gameServerSnapshot.findFirst.mockResolvedValue(snapshot as GameServerSnapshot);
  return snapshot;
};

const mockSecondSnapshot = (baseDate: Date, clients: GameServerClient[]) => {
  const snapshot = newSnapshot(2, baseDate, clients);
  prismaMock.gameServerSnapshot.findUniqueOrThrow.mockResolvedValue(snapshot as GameServerSnapshot);
  return snapshot;
};

function checkPlayTimes(expectedPlayerPlayTimes: number[], expectedClanPlayTimes: number[], expectedClanPlayerPlayTimes: number[], expectedGlobalPlayTime: number) {
  // Check player updates
  expectedPlayerPlayTimes.forEach((playTime, index) => {
    expect(prismaMock.player.update).toHaveBeenCalledWith({
      where: { name: `player${index}` },
      data: { playTime: { increment: playTime } }
    });

    expect(prismaMock.playerInfoMap.upsert).toHaveBeenCalledWith(expect.objectContaining({
      where: {
        playerName_mapId: {
          mapId: 1,
          playerName: `player${index}`
        }
      },
      update: { playTime: { increment: playTime } }
    }));

    expect(prismaMock.playerInfoGameType.upsert).toHaveBeenCalledWith(expect.objectContaining({
      where: {
        playerName_gameTypeName: {
          gameTypeName: 'gameType',
          playerName: `player${index}`
        }
      },
      update: { playTime: { increment: playTime } }
    }));
  });

  // Check clan updates
  expectedClanPlayTimes.forEach((playTime, index) => {
    expect(prismaMock.clan.update).toHaveBeenCalledWith({
      where: { name: `clan${index}` },
      data: { playTime: { increment: playTime } }
    });

    expect(prismaMock.clanInfoMap.upsert).toHaveBeenCalledWith(expect.objectContaining({
      where: {
        clanName_mapId: {
          mapId: 1,
          clanName: `clan${index}`
        }
      },
      update: { playTime: { increment: playTime } }
    }));

    expect(prismaMock.clanInfoGameType.upsert).toHaveBeenCalledWith(expect.objectContaining({
      where: {
        clanName_gameTypeName: {
          gameTypeName: 'gameType',
          clanName: `clan${index}`
        }
      },
      update: { playTime: { increment: playTime } }
    }));
  });

  // Check clan-player updates
  expectedClanPlayerPlayTimes.forEach((playTime) => {
    expect(prismaMock.clanPlayerInfo.upsert).toHaveBeenCalledWith(expect.objectContaining({
      update: { playTime: { increment: playTime } }
    }));
  });

  // Check global updates
  expect(prismaMock.gameType.update).toHaveBeenCalledWith({
    where: { name: 'gameType' },
    data: { playTime: { increment: expectedGlobalPlayTime } }
  });

  expect(prismaMock.map.update).toHaveBeenCalledWith({
    where: { id: 1 },
    data: { playTime: { increment: expectedGlobalPlayTime } }
  });

  expect(prismaMock.gameServer.update).toHaveBeenCalledWith({
    where: { id: 1 },
    data: { playTime: { increment: expectedGlobalPlayTime } }
  });
}

test('Single snapshot', async () => {
  const baseDate = new Date();
  const clients = [newClient('player0', 'clan0')];
  const snapshot = mockSecondSnapshot(baseDate, clients);
  prismaMock.gameServerSnapshot.findFirst.mockResolvedValue(null);

  await updatePlayTime(snapshot.id);
  checkPlayTimes([0], [0], [0], 0);
});

test('One player, no clan', async () => {
  const baseDate = new Date();
  const clients = [newClient('player0')];

  mockFirstSnapshot(baseDate, clients);
  mockSecondSnapshot(addMinutes(baseDate, 5), clients);

  await updatePlayTime(2);
  checkPlayTimes([5 * 60], [], [], 5 * 60);
});

test('One player, one clan', async () => {
  const baseDate = new Date();
  const clients = [newClient('player0', 'clan0')];

  mockFirstSnapshot(baseDate, clients);
  mockSecondSnapshot(addMinutes(baseDate, 5), clients);

  await updatePlayTime(2);
  checkPlayTimes([5 * 60], [5 * 60], [5 * 60], 5 * 60);
});

test('Two players, same clan', async () => {
  const baseDate = new Date();
  const clients = [
    newClient('player0', 'clan0'),
    newClient('player1', 'clan0')
  ];

  mockFirstSnapshot(baseDate, clients);
  mockSecondSnapshot(addMinutes(baseDate, 5), clients);

  await updatePlayTime(2);
  checkPlayTimes([5 * 60, 5 * 60], [10 * 60], [5 * 60, 5 * 60], 10 * 60);
});

test('Two players, different clan', async () => {
  const baseDate = new Date();
  const clients = [
    newClient('player0', 'clan0'),
    newClient('player1', 'clan1')
  ];

  mockFirstSnapshot(baseDate, clients);
  mockSecondSnapshot(addMinutes(baseDate, 5), clients);

  await updatePlayTime(2);
  checkPlayTimes([5 * 60, 5 * 60], [5 * 60, 5 * 60], [5 * 60, 5 * 60], 2 * 5 * 60);
});
