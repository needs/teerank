import { prisma } from '../prisma';
import { clearDatabase, runJobNTimes } from '../../testSetup';
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
  await createSnapshot([100, 100]);
  await runJobNTimes(2, rankPlayers);
  await checkRatings([0, 0], [0, 0]);
});

test('Different map', async () => {
  await createSnapshot([100, 100]);
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

  await runJobNTimes(3, rankPlayers);

  await checkRatings([0, 0], [0, 0, 0, 0]);
});

test('Different game type', async () => {
  await createSnapshot([100, 100]);
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

  await runJobNTimes(3, rankPlayers);

  await checkRatings([0, 0, 0, 0], [0, 0, 0, 0]);
});

test('Not enough players', async () => {
  await createSnapshot([100]);
  await createSnapshot([101]);

  await runJobNTimes(3, rankPlayers);

  await checkRatings([0], [0]);
});

test('Negative score average', async () => {
  await createSnapshot([100, 100]);
  await createSnapshot([98, 98]);

  await runJobNTimes(3, rankPlayers);

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

  await runJobNTimes(3, rankPlayers);

  await checkRatings([0, 0], [0, 0]);
});

test('Two players', async () => {
  await createSnapshot([100, 100]);
  await createSnapshot([99, 101]);

  await runJobNTimes(3, rankPlayers);

  await checkRatings([-12.5, 12.5], [-12.5, 12.5]);
});

test('Rank order', async () => {
  await createSnapshot([100, 100]);
  await createSnapshot([99, 101]);

  await runJobNTimes(3, rankPlayers);

  await checkRatings([-12.5, 12.5], [-12.5, 12.5]);
});
