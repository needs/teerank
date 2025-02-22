import { prismaMock } from '../../test/mockPrisma';
import { GameServerClient, GameServerSnapshot, Map, GameType, RankMethod } from '@prisma/client';
import { rankPlayer } from '../workers/rankPlayer';
import { addHours } from 'date-fns';

type MockedGameServerSnapshot = GameServerSnapshot & {
  clients: GameServerClient[],
  map: Map & { gameType: GameType }
};

function createSnapshot(id: number, createdAt: Date, scores: number[]): MockedGameServerSnapshot {
  return {
    id,
    name: 'snapshot',
    version: 'version',
    createdAt,
    gameServerId: 1,
    mapId: 1,
    maxClients: scores.length,
    numClients: scores.length,
    maxPlayers: scores.length,
    numPlayers: scores.length,
    clients: scores.map((score, index) => ({
      id: index,
      snapshotId: id,
      playerName: `player${index}`,
      clanName: null,
      score,
      inGame: true,
      country: 0,
    })),
    map: {
      id: 1,
      name: 'map',
      gameTypeName: 'gameType',
      createdAt: new Date(),
      playTime: BigInt(0),
      clanCount: 0,
      playerCount: 0,
      gameServerCount: 0,
      gameType: {
        name: 'gameType',
        rankMethod: RankMethod.ELO,
        createdAt: new Date(),
        playTime: BigInt(0),
        playerCount: 0,
        clanCount: 0,
        gameServerCount: 0,
        mapCount: 0,
      }
    }
  };
}

function mockSnapshot(snapshot: MockedGameServerSnapshot, previousSnapshot: MockedGameServerSnapshot | null = null) {
  prismaMock.gameServerSnapshot.findUniqueOrThrow.mockResolvedValue(snapshot);
  prismaMock.gameServerSnapshot.findFirst.mockResolvedValue(previousSnapshot);

  // Mock initial player info creation
  snapshot.clients.forEach((client, index) => {
    prismaMock.playerInfoMap.upsert.mockResolvedValueOnce({
      id: index,
      playerName: client.playerName,
      mapId: 1,
      rating: 0,
      createdAt: new Date(),
      updatedAt: new Date(),
      playTime: BigInt(0),
    });

    prismaMock.playerInfoGameType.upsert.mockResolvedValueOnce({
      id: index,
      playerName: client.playerName,
      gameTypeName: 'gameType',
      rating: 0,
      createdAt: new Date(),
      updatedAt: new Date(),
      playTime: BigInt(0),
    });
  });
}

function checkRatings(expectedRatingsGameType: number[], expectedRatingsMap: number[]) {
  expectedRatingsGameType.forEach((rating, index) => {
    expect(prismaMock.playerInfoGameType.update).toHaveBeenCalledWith({
      where: { id: index },
      data: { rating: { increment: rating } }
    });
  });

  expectedRatingsMap.forEach((rating, index) => {
    expect(prismaMock.playerInfoMap.update).toHaveBeenCalledWith({
      where: { id: index },
      data: { rating: { increment: rating } }
    });
  });
}

test('Only one snapshot', async () => {
  const snapshot = createSnapshot(1, new Date(), [100, 100]);
  mockSnapshot(snapshot);
  await rankPlayer(snapshot.id);
  checkRatings([], []);
});

test('Different map', async () => {
  const baseDate = new Date();
  const snapshot1 = createSnapshot(1, baseDate, [100, 100]);
  const snapshot2 = createSnapshot(2, baseDate, [99, 101]);
  snapshot2.mapId = 2;

  mockSnapshot(snapshot1);
  mockSnapshot(snapshot2, snapshot1);

  await rankPlayer(snapshot1.id);
  await rankPlayer(snapshot2.id);
  checkRatings([], []);
});

test('Big time gap', async () => {
  const baseDate = new Date();
  const snapshot1 = createSnapshot(1, baseDate, [100, 100]);
  const snapshot2 = createSnapshot(2, addHours(baseDate, 1), [99, 101]);

  mockSnapshot(snapshot1);
  mockSnapshot(snapshot2, snapshot1);

  await rankPlayer(snapshot1.id);
  await rankPlayer(snapshot2.id);
  checkRatings([], []);
});

test('Two players', async () => {
  const baseDate = new Date();
  const snapshot1 = createSnapshot(1, baseDate, [100, 100]);
  const snapshot2 = createSnapshot(2, baseDate, [99, 101]);

  mockSnapshot(snapshot1);
  mockSnapshot(snapshot2, snapshot1);

  await rankPlayer(snapshot1.id);
  await rankPlayer(snapshot2.id);
  checkRatings([-12.5, 12.5], [-12.5, 12.5]);
});
