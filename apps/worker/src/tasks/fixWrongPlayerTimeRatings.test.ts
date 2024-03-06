import { prisma } from '../prisma';
import { clearDatabase, runJobNTimes } from '../../testSetup';
import { fixWrongPlayerTimeRatings } from '../tasks/fixWrongPlayerTimeRatings';
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

async function createRatings(ratings: number[]) {
  const map = await prisma.map.upsert({
    where: {
      name_gameTypeName: {
        name: 'map',
        gameTypeName: 'gameType',
      }
    },
    select: {
      id: true,
    },
    update: {},
    create: {
      name: 'map',
      gameType: {
        create: {
          name: 'gameType',
          rankMethod: RankMethod.TIME,
        },
      },
    },
  });

  await prisma.player.createMany({
    data: ratings.map((_, index) => ({
      name: `player${index}`,
    })),

    skipDuplicates: true,
  });

  return await prisma.playerInfoMap.createMany({
    data: ratings.map((rating, index) => ({
      playerName: `player${index}`,
      mapId: map.id,
      rating,
    })),
  });

}

async function checkRatings(expectedRatings: number[]) {
  const mapPlayerInfos = await prisma.playerInfoMap.findMany({
    select: {
      rating: true,
    },
    orderBy: {
      playerName: 'asc',
    },
  });

  expect(mapPlayerInfos.map(playerInfo => playerInfo.rating).filter(rating => rating !== null)).toEqual(expectedRatings);
}

test('Already correct rating', async () => {
  await createSnapshot([-10]);
  await createRatings([-10]);
  await runJobNTimes(1, fixWrongPlayerTimeRatings);
  await checkRatings([-10]);
});

test('Wrong rating', async () => {
  await createSnapshot([-11]);
  await createSnapshot([-10]);
  await createRatings([0]);
  await runJobNTimes(2, fixWrongPlayerTimeRatings);
  await checkRatings([-10]);
});

test('Wrong rating and wrong snapshot', async () => {
  await createSnapshot([0]);
  await createRatings([0]);
  await runJobNTimes(2, fixWrongPlayerTimeRatings);
  await checkRatings([]);
});

test('Wrong rating and no snapshot', async () => {
  await createRatings([0]);
  await runJobNTimes(2, fixWrongPlayerTimeRatings);
  await checkRatings([]);
});
