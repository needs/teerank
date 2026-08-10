import { prismaMock } from '../../test/mockPrisma';
import { GameServerClient, GameServerSnapshot, GameType, Map, PlayerInfoMap, RankMethod } from '@prisma/client';
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
        id: 1,
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

  // Info rows exist without a rating yet.
  const playerInfoMaps: PlayerInfoMap[] = snapshot.clients.map((client, index) => ({
    id: index,
    playerName: client.playerName,
    rating: null,
    mapId: snapshot.mapId,
    playTime: BigInt(0),
    createdAt: new Date(),
    updatedAt: new Date(),
  }));

  prismaMock.playerInfoMap.findMany.mockResolvedValue(playerInfoMaps);

  return snapshot;
}

// New times are written as one multi-row UPDATE with (playerName, rating)
// pairs as the interpolated values, plus the mapId as the last parameter.
function timeUpdate(): { values: unknown[], mapId: unknown } | null {
  const call = prismaMock.$executeRaw.mock.calls.find(
    (args) => (args[0] as unknown as ReadonlyArray<string>).join('?').includes('UPDATE "PlayerInfoMap" SET')
  );

  if (call === undefined) {
    return null;
  }

  return {
    values: (call[1] as { values: unknown[] }).values,
    mapId: call[2],
  };
}

function checkRatings(expectedRatings: (number | null)[]) {
  const update = timeUpdate();

  if (expectedRatings.every((rating) => rating === null)) {
    expect(update).toBeNull();
    return;
  }

  expect(update).not.toBeNull();
  expect(update?.mapId).toBe(1);

  expectedRatings.forEach((rating, index) => {
    if (rating !== null) {
      expect(update?.values).toEqual(expect.arrayContaining([`player${index}`, rating]));
    }
  });
}

afterEach(() => {
  jest.clearAllMocks();
});

test('Positive and negative time', async () => {
  const snapshot = createSnapshot([10, -10]);
  mockSnapshot(snapshot);
  await rankPlayer({ snapshotId: snapshot.id });
  checkRatings([-10, -10]);
});

test('Time increase', async () => {
  const snapshot1 = createSnapshot([10]);
  mockSnapshot(snapshot1);
  await rankPlayer({ snapshotId: snapshot1.id });
  checkRatings([-10]);
  jest.clearAllMocks();

  const snapshot2 = createSnapshot([30]);
  mockSnapshot(snapshot2);
  await rankPlayer({ snapshotId: snapshot2.id });
  checkRatings([-30]);
});

test('Time decrease', async () => {
  const snapshot1 = createSnapshot([30]);
  mockSnapshot(snapshot1);
  await rankPlayer({ snapshotId: snapshot1.id });
  checkRatings([-30]);
  jest.clearAllMocks();

  const snapshot2 = createSnapshot([10]);
  mockSnapshot(snapshot2);
  await rankPlayer({ snapshotId: snapshot2.id });
  checkRatings([-10]);
});

test('Maximum time', async () => {
  const snapshot = createSnapshot([9999, -9999]);
  mockSnapshot(snapshot);
  await rankPlayer({ snapshotId: snapshot.id });
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
  await rankPlayer({ snapshotId: snapshot.id });
  checkRatings([null]);
});
