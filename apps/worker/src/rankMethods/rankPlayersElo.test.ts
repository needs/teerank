import { prismaMock } from '../../test/mockPrisma';
import { GameServerClient, GameServerSnapshot, Map, GameType, RankMethod, PlayerInfoMap, PlayerInfoGameType } from '@prisma/client';
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
        id: 1,
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

  // Info rows already exist with a 0 rating, so the ranking reads them
  // instead of creating them.
  const infoRows = snapshot.clients.map((client, index) => ({
    id: index,
    playerName: client.playerName,
    rating: 0,
  }));

  prismaMock.playerInfoMap.findMany.mockResolvedValue(infoRows as PlayerInfoMap[]);
  prismaMock.playerInfoGameType.findMany.mockResolvedValue(infoRows as PlayerInfoGameType[]);
}

// Rating increments are written as one multi-row UPDATE per table with
// (id, eloDelta) pairs as the interpolated values.
function rawUpdateValues(table: string): unknown[] | null {
  const call = prismaMock.$executeRaw.mock.calls.find(
    (args) => (args[0] as unknown as ReadonlyArray<string>).join('?').includes(`UPDATE "${table}" SET`)
  );

  if (call === undefined) {
    return null;
  }

  return (call[1] as { values: unknown[] }).values;
}

function checkRatings(expectedRatingsGameType: number[], expectedRatingsMap: number[]) {
  const gameTypeValues = rawUpdateValues('PlayerInfoGameType');
  const mapValues = rawUpdateValues('PlayerInfoMap');

  if (expectedRatingsGameType.length === 0) {
    expect(gameTypeValues).toBeNull();
  } else {
    expectedRatingsGameType.forEach((rating, index) => {
      expect(gameTypeValues).toEqual(expect.arrayContaining([index, rating]));
    });
  }

  if (expectedRatingsMap.length === 0) {
    expect(mapValues).toBeNull();
  } else {
    expectedRatingsMap.forEach((rating, index) => {
      expect(mapValues).toEqual(expect.arrayContaining([index, rating]));
    });
  }
}

test('Only one snapshot', async () => {
  const snapshot = createSnapshot(1, new Date(), [100, 100]);
  mockSnapshot(snapshot);
  await rankPlayer({ snapshotId: snapshot.id });
  checkRatings([], []);
});

test('Different map', async () => {
  const baseDate = new Date();
  const snapshot1 = createSnapshot(1, baseDate, [100, 100]);
  const snapshot2 = createSnapshot(2, baseDate, [99, 101]);
  snapshot2.mapId = 2;

  mockSnapshot(snapshot1);
  await rankPlayer({ snapshotId: snapshot1.id });

  mockSnapshot(snapshot2, snapshot1);
  await rankPlayer({ snapshotId: snapshot2.id });

  checkRatings([], []);
});

test('Big time gap', async () => {
  const baseDate = new Date();
  const snapshot1 = createSnapshot(1, baseDate, [100, 100]);
  const snapshot2 = createSnapshot(2, addHours(baseDate, 1), [99, 101]);

  mockSnapshot(snapshot1);
  await rankPlayer({ snapshotId: snapshot1.id });

  mockSnapshot(snapshot2, snapshot1);
  await rankPlayer({ snapshotId: snapshot2.id });

  checkRatings([], []);
});

test('Two players', async () => {
  const baseDate = new Date();
  const snapshot1 = createSnapshot(1, baseDate, [100, 100]);
  const snapshot2 = createSnapshot(2, baseDate, [99, 101]);

  mockSnapshot(snapshot1);
  await rankPlayer({ snapshotId: snapshot1.id });

  mockSnapshot(snapshot2, snapshot1);
  await rankPlayer({ snapshotId: snapshot2.id });

  checkRatings([-12.5, 12.5], [-12.5, 12.5]);
});
