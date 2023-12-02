import { prisma } from '../prisma';
import { clearDatabase } from '../../testSetup';
import { rankPlayers } from './rankPlayers';
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

async function checkRatings(expectedRatings: number[]) {
  const mapPlayerInfos = await prisma.playerInfo.findMany({
    select: {
      rating: true,
    },
    where: {
      map: {
        isNot: null,
      }
    },
    orderBy: {
      playerName: 'asc',
    },
  });

  const gameTypePlayerInfos = await prisma.playerInfo.findMany({
    select: {
      rating: true,
    },
    where: {
      gameType: {
        isNot: null,
      }
    },
    orderBy: {
      playerName: 'asc',
    },
  });

  expect(mapPlayerInfos.map(playerInfo => playerInfo.rating).filter(rating => rating !== null)).toEqual(expectedRatings);
  expect(gameTypePlayerInfos.map(playerInfo => playerInfo.rating).filter(rating => rating !== null)).toEqual(expectedRatings);
}

test('Only one snapshot', async () => {
  await createSnapshot([100, 100]);
  await rankPlayers();
  await checkRatings([]);
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

  await rankPlayers();

  await checkRatings([]);
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

  await rankPlayers();

  await checkRatings([]);
});

test('Not enough players', async () => {
  await createSnapshot([100]);
  await createSnapshot([101]);

  await rankPlayers();

  await checkRatings([]);
});

test('Negative score average', async () => {
  await createSnapshot([100, 100]);
  await createSnapshot([98, 98]);

  await rankPlayers();

  await checkRatings([]);
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

  await rankPlayers();

  await checkRatings([]);
});

test('Two players', async () => {
  await createSnapshot([100, 100]);
  await createSnapshot([99, 101]);

  await rankPlayers();

  await checkRatings([-12.5, 12.5]);
});

// TODO: test rankPlayers
// TODO: Setup test in CI and make sure DATABASE_URL is set correctly
