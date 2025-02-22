import { prismaMock } from '../../test/mockPrisma';
import { GameServerSnapshot, RankMethod } from '@prisma/client';
import { rankPlayer } from '../workers/rankPlayer';

// Helper to create a mock snapshot
function createMockSnapshot(scores: number[]) {
  const snapshot = {
    id: 1,
    clients: scores.map((score, index) => ({
      playerName: `player${index}`,
      score,
      inGame: true,
    })),
    mapId: 1,
    map: {
      name: 'map',
      gameType: {
        name: 'gameType',
        rankMethod: RankMethod.TIME,
      }
    }
  };

  prismaMock.gameServerSnapshot.findUniqueOrThrow.mockResolvedValue(snapshot as unknown as GameServerSnapshot);

  scores.forEach((_, index) => {
    prismaMock.playerInfoMap.upsert.mockResolvedValueOnce({
      id: index,
      playerName: `player${index}`,
      rating: null,
      mapId: 1,
      createdAt: new Date(),
      updatedAt: new Date(),
      playTime: BigInt(0),
    });
  });

  return snapshot;
}

// Helper to mock existing player info and verify updates
async function checkRatings(expectedRatings: (number | null)[]) {
  // For each player, mock the upsert response
  expectedRatings.forEach((_, index) => {
    prismaMock.playerInfoMap.upsert.mockResolvedValueOnce({
      id: index,
      playerName: `player${index}`,
      rating: null,
      mapId: 1,
      createdAt: new Date(),
      updatedAt: new Date(),
      playTime: BigInt(0),
    });
  });

  // Verify the update calls were made with correct values
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
  const snapshot = createMockSnapshot([10, -10]);
  await rankPlayer(snapshot.id);
  await checkRatings([-10, -10]);
});

test('Time increase', async () => {
  const snapshot1 = createMockSnapshot([10]);
  await rankPlayer(snapshot1.id);
  await checkRatings([-10]);

  const snapshot2 = createMockSnapshot([30]);
  await rankPlayer(snapshot2.id);
  await checkRatings([-10]);
});

test('Time decrease', async () => {
  const snapshot1 = createMockSnapshot([30]);
  await rankPlayer(snapshot1.id);
  await checkRatings([-30]);

  const snapshot2 = createMockSnapshot([10]);
  await rankPlayer(snapshot2.id);
  await checkRatings([-10]);
});

test('Maximum time', async () => {
  const snapshot = createMockSnapshot([9999, -9999]);
  await rankPlayer(snapshot.id);
  await checkRatings([null, null]);
});

test('Connecting player', async () => {
  const snapshot = createMockSnapshot([]);
  // Update the snapshot to include a connecting player
  snapshot.clients.push({
    playerName: '(connecting)',
    score: 10,
    inGame: true,
  });

  await rankPlayer(snapshot.id);
  await checkRatings([null]);
});
