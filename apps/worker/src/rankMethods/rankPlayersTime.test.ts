import { prismaMock } from '../../test/mockPrisma';
import { GameServerClient, GameServerSnapshot, GameType, Map, RankMethod } from '@prisma/client';
import { rankPlayer } from '../workers/rankPlayer';

type MockedGameServerSnapshot = GameServerSnapshot & { clients: GameServerClient[], map: Map & { gameType: GameType } };

function createSnapshot(scores: number[]): MockedGameServerSnapshot {
  return {
    id: 1,
    name: 'snapshot',
    version: 'version',
    createdAt: new Date(),
    gameServerId: 1,
    mapId: 1,
    numPlayers: 0,
    maxPlayers: 0,
    numClients: 0,
    maxClients: 0,
    clients: scores.map((score, index) => ({
      id: index,
      snapshotId: 1,
      playerName: `player${index}`,
      clanName: null,
      score,
      country: 0,
      inGame: true,
    })),
    map: {
      name: 'map',
      id: 1,
      createdAt: new Date(),
      clanCount: 0,
      gameServerCount: 0,
      playerCount: 0,
      playTime: BigInt(0),
      gameTypeName: 'gameType',
      gameType: {
        name: 'gameType',
        rankMethod: RankMethod.TIME,
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

function mockSnapshot(snapshot: MockedGameServerSnapshot) {
  prismaMock.gameServerSnapshot.findUniqueOrThrow.mockResolvedValue(snapshot);

  snapshot.clients.forEach((client, index) => {
    prismaMock.playerInfoMap.upsert.mockResolvedValueOnce({
      id: index,
      playerName: client.playerName,
      rating: null,
      mapId: snapshot.mapId,
      createdAt: new Date(),
      updatedAt: new Date(),
      playTime: BigInt(0),
    });
  });

  return snapshot;
}

function checkRatings(expectedRatings: (number | null)[]) {
  expectedRatings.forEach((rating, index) => {
    if (rating !== null) {
      expect(prismaMock.playerInfoMap.update).toHaveBeenCalledWith({
        where: {
          playerName_mapId: {
            playerName: `player${index}`,
            mapId: 1,
          },
        },
        data: {
          rating: rating,
        },
      });
    }
  });
}

test('Positive and negative time', async () => {
  const snapshot = createSnapshot([10, -10]);
  mockSnapshot(snapshot);
  await rankPlayer(snapshot.id);
  checkRatings([-10, -10]);
});

test('Time increase', async () => {
  const snapshot1 = createSnapshot([10]);
  mockSnapshot(snapshot1);
  await rankPlayer(snapshot1.id);
  checkRatings([-10]);

  const snapshot2 = createSnapshot([30]);
  mockSnapshot(snapshot2);
  await rankPlayer(snapshot2.id);
  checkRatings([-30]);
});

test('Time decrease', async () => {
  const snapshot1 = createSnapshot([30]);
  mockSnapshot(snapshot1);
  await rankPlayer(snapshot1.id);
  checkRatings([-30]);

  const snapshot2 = createSnapshot([10]);
  mockSnapshot(snapshot2);
  await rankPlayer(snapshot2.id);
  checkRatings([-10]);
});

test('Maximum time', async () => {
  const snapshot = createSnapshot([9999, -9999]);
  mockSnapshot(snapshot);
  await rankPlayer(snapshot.id);
  checkRatings([null, null]);
});

test('Connecting player', async () => {
  const snapshot = createSnapshot([]);
  snapshot.clients.push({
    id: 1,
    snapshotId: 1,
    playerName: '(connecting)',
    clanName: null,
    score: 10,
    country: 0,
    inGame: true,
  });
  mockSnapshot(snapshot);
  await rankPlayer(snapshot.id);
  checkRatings([null]);
});
