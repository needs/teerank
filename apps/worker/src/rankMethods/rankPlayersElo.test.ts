import { prisma } from '../prisma';
import { clearDatabase } from '../../testSetup';
import { rankPlayers } from '../tasks/rankPlayers';
import { addHours } from 'date-fns';
import { RankMethod } from '@prisma/client';

beforeEach(async () => {
  await clearDatabase();
});

async function createSnapshot(scores: number[]) {
  await prisma.player.createMany({
    data: scores.map((_, index) => ({
      name: `player${index}`,
    })),

    skipDuplicates: true,
  });

  return await prisma.gameServerSnapshot.create({
    data: {
      map: {
        connectOrCreate: {
          where: {
            name_gameTypeName: {
              name: 'map',
              gameTypeName: 'gameType',
            },
          },
          create: {
            name: 'map',
            gameType: {
              create: {
                name: 'gameType',
                rankMethod: RankMethod.ELO,
              },
            },
          },
        },
      },

      maxClients: scores.length,
      numClients: scores.length,
      maxPlayers: scores.length,
      numPlayers: scores.length,

      clients: {

        createMany: {
          data: scores.map((score, index) => ({
            playerName: `player${index}`,
            score,
            inGame: true,
            country: 0,
          })),
        }
      },

      name: 'snapshot',
      version: 'version',

      gameServer: {
        connectOrCreate: {
          where: {
            ip_port: {
              ip: 'localhost',
              port: 8303,
            }
          },
          create: {
            ip: 'localhost',
            port: 8303,
          }
        }
      }
    }
  });
}

async function checkRatings(expectedRatingsGameType: number[], expectedRatingsMap: number[]) {
  const gameTypePlayerInfos = await prisma.playerInfoGameType.findMany({
    select: {
      rating: true,
    },
    orderBy: {
      playerName: 'asc',
    },
  });

  const mapPlayerInfos = await prisma.playerInfoMap.findMany({
    select: {
      rating: true,
    },
    orderBy: {
      playerName: 'asc',
    },
  });

  expect(gameTypePlayerInfos.map(playerInfo => playerInfo.rating).filter(rating => rating !== null)).toEqual(expectedRatingsGameType);
  expect(mapPlayerInfos.map(playerInfo => playerInfo.rating).filter(rating => rating !== null)).toEqual(expectedRatingsMap);
}

test('Only one snapshot', async () => {
  const snapshot = await createSnapshot([100, 100]);
  await rankPlayers(snapshot.id, snapshot.id);
  await checkRatings([0, 0], [0, 0]);
});

test('Different map', async () => {
  const snapshot1 = await createSnapshot([100, 100]);
  const snapshot2 = await createSnapshot([99, 101]);

  await prisma.gameServerSnapshot.update({
    where: {
      id: snapshot2.id,
    },
    data: {
      map: {
        create: {
          name: 'map2',
          gameType: {
            connect: {
              name: 'gameType',
            },
          },
        },
      },
    },
  });

  await rankPlayers(snapshot1.id, snapshot2.id);

  await checkRatings([0, 0], [0, 0, 0, 0]);
});

test('Different game type', async () => {
  const snapshot1 = await createSnapshot([100, 100]);
  const snapshot2 = await createSnapshot([99, 101]);

  await prisma.gameServerSnapshot.update({
    where: {
      id: snapshot2.id,
    },
    data: {
      map: {
        create: {
          name: 'map',
          gameType: {
            create: {
              name: 'gameType2',
              rankMethod: RankMethod.ELO,
            },
          },
        },
      },
    },
  });

  await rankPlayers(snapshot1.id, snapshot2.id);

  await checkRatings([0, 0, 0, 0], [0, 0, 0, 0]);
});

test('Not enough players', async () => {
  const snapshot1 = await createSnapshot([100]);
  const snapshot2 = await createSnapshot([101]);

  await rankPlayers(snapshot1.id, snapshot2.id);

  await checkRatings([0], [0]);
});

test('Negative score average', async () => {
  const snapshot1 = await createSnapshot([100, 100]);
  const snapshot2 = await createSnapshot([98, 98]);

  await rankPlayers(snapshot1.id, snapshot2.id);

  await checkRatings([0, 0], [0, 0]);
});

test('Big time gap', async () => {
  const snapshot1 = await createSnapshot([100, 100]);
  const snapshot2 = await createSnapshot([99, 101]);

  await prisma.gameServerSnapshot.update({
    where: {
      id: snapshot2.id,
    },
    data: {
      createdAt: addHours(snapshot1.createdAt, 1),
    },
  });

  await rankPlayers(snapshot1.id, snapshot2.id);

  await checkRatings([0, 0], [0, 0]);
});

test('Two players', async () => {
  const snapshot1 = await createSnapshot([100, 100]);
  const snapshot2 = await createSnapshot([99, 101]);

  await rankPlayers(snapshot1.id, snapshot2.id);

  await checkRatings([-12.5, 12.5], [-12.5, 12.5]);
});

test('Rank order', async () => {
  const snapshot1 = await createSnapshot([100, 100]);
  const snapshot2 = await createSnapshot([99, 101]);

  await rankPlayers(snapshot1.id, snapshot2.id);

  await checkRatings([-12.5, 12.5], [-12.5, 12.5]);
});
