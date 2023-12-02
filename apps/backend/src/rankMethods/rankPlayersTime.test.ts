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
                rankMethod: RankMethod.TIME,
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
  expect(gameTypePlayerInfos.map(playerInfo => playerInfo.rating).filter(rating => rating !== null)).toEqual([]);
}

test('Positive and negative time', async () => {
  await createSnapshot([10, -10]);
  await rankPlayers();
  await checkRatings([-10, -10]);
});

test('Time increase', async () => {
  await createSnapshot([10]);
  await createSnapshot([30]);
  await rankPlayers();
  await checkRatings([-10]);
});

test('Time decrease', async () => {
  await createSnapshot([30]);
  await createSnapshot([10]);
  await rankPlayers();
  await checkRatings([-10]);
});

test('Maximum time', async () => {
  await createSnapshot([9999, -9999]);
  await rankPlayers();
  await checkRatings([]);
});

test('Connecting player', async () => {
  const snapshot = await createSnapshot([]);

  await prisma.gameServerSnapshot.update({
    where: {
      id: snapshot.id,
    },
    data: {
      clients: {
        create: {
          player: {
            create: {
              name: '(connecting)',
            },
          },
          score: 10,
          inGame: true,
          country: 0,
        },
      },
    },
  });

  await rankPlayers();
  await checkRatings([]);
});
