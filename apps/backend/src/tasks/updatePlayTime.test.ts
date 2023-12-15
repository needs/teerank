import { addMinutes } from "date-fns";
import { clearDatabase } from "../../testSetup";
import { prisma } from "../prisma";
import { updatePlayTimes } from "./updatePlayTime";

beforeEach(async () => {
  await clearDatabase();
});

async function createSnapshot(createdAt: Date, numPlayers: number, numClans: number) {
  const playerIndices = [...Array(numPlayers).keys()];
  const clansIndices = [...Array(numClans).keys()];

  await prisma.player.createMany({
    data: playerIndices.map((index) => ({
      name: `player${index}`,
    })),

    skipDuplicates: true,
  });

  if (numClans > 0) {
    await prisma.clan.createMany({
      data: clansIndices.map((index) => ({
        name: `clan${index}`,
      })),

      skipDuplicates: true,
    });
  }

  return await prisma.gameServerSnapshot.create({
    data: {
      createdAt,

      clients: {
        createMany: {
          data: playerIndices.map((index) => ({
            playerName: `player${index}`,
            clanName: numClans <= 0 ? undefined : `clan${index % numClans}`,
            score: 0,
            inGame: true,
            country: 0,
          })),
        }
      },

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
              },
            },
          },
        },
      },

      maxClients: numPlayers,
      numClients: numPlayers,
      maxPlayers: numPlayers,
      numPlayers: numPlayers,

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

async function checkPlayTimes(expectedPlayerPlayTimes: number[], expectedClanPlayTimes: number[], expectedClanPlayerPlayTimes: number[], gameTypeAndMapPlayTime: number) {
  const players = await prisma.player.findMany({
    select: {
      playTime: true,
    },
    orderBy: {
      name: 'asc',
    },
  });

  const mapPlayerInfos = await prisma.playerInfo.findMany({
    select: {
      playTime: true,
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
      playTime: true,
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

  const clans = await prisma.clan.findMany({
    select: {
      playTime: true,
    },
    orderBy: {
      name: 'asc',
    },
  });

  const mapClanInfos = await prisma.clanInfo.findMany({
    select: {
      playTime: true,
    },
    where: {
      map: {
        isNot: null,
      }
    },
    orderBy: {
      clanName: 'asc',
    },
  });

  const gameTypeClanInfos = await prisma.clanInfo.findMany({
    select: {
      playTime: true,
    },
    where: {
      gameType: {
        isNot: null,
      }
    },
    orderBy: {
      clanName: 'asc',
    },
  });

  const clanPlayerInfos = await prisma.clanPlayerInfo.findMany({
    select: {
      playTime: true,
    },
    orderBy: {
      clanName: 'asc',
    },
  });

  const gameTypePlayTime = await prisma.gameType.findUniqueOrThrow({
    where: {
      name: 'gameType',
    },
    select: {
      playTime: true,
    },
  });

  const mapPlayTime = await prisma.map.findUniqueOrThrow({
    where: {
      name_gameTypeName: {
        name: 'map',
        gameTypeName: 'gameType',
      },
    },
    select: {
      playTime: true,
    },
  });

  expect(players.map(playerInfo => playerInfo.playTime)).toEqual(expectedPlayerPlayTimes);
  expect(mapPlayerInfos.map(playerInfo => playerInfo.playTime)).toEqual(expectedPlayerPlayTimes);
  expect(gameTypePlayerInfos.map(playerInfo => playerInfo.playTime)).toEqual(expectedPlayerPlayTimes);

  expect(clans.map(clanInfo => clanInfo.playTime)).toEqual(expectedClanPlayTimes);
  expect(mapClanInfos.map(clanInfo => clanInfo.playTime)).toEqual(expectedClanPlayTimes);
  expect(gameTypeClanInfos.map(clanInfo => clanInfo.playTime)).toEqual(expectedClanPlayTimes);

  expect(clanPlayerInfos.map(clanPlayerInfo => clanPlayerInfo.playTime)).toEqual(expectedClanPlayerPlayTimes);

  expect(gameTypePlayTime.playTime).toEqual(gameTypeAndMapPlayTime);
  expect(mapPlayTime.playTime).toEqual(gameTypeAndMapPlayTime);
}

test('Single snapshot', async () => {
  await createSnapshot(new Date(), 1, 1);
  await updatePlayTimes();
  await checkPlayTimes([0], [0], [0], 0);
});

test('One player, no clan', async () => {
  const snapshot = await createSnapshot(new Date(), 1, 0);
  await createSnapshot(addMinutes(snapshot.createdAt, 5), 1, 0);

  await updatePlayTimes();
  await checkPlayTimes([5 * 60], [], [], 5 * 60);
});

test('One player, one clan', async () => {
  const snapshot = await createSnapshot(new Date(), 1, 1);
  await createSnapshot(addMinutes(snapshot.createdAt, 5), 1, 1);

  await updatePlayTimes();
  await checkPlayTimes([5 * 60], [5 * 60], [5 * 60], 5 * 60);
});

test('Two players, same clan', async () => {
  const snapshot = await createSnapshot(new Date(), 2, 1);
  await createSnapshot(addMinutes(snapshot.createdAt, 5), 2, 1);

  await updatePlayTimes();
  await checkPlayTimes([5 * 60, 5 * 60], [2 * 5 * 60], [5 * 60, 5 * 60], 2 * 5 * 60);
});

test('Two players, different clan', async () => {
  const snapshot = await createSnapshot(new Date(), 2, 2);
  await createSnapshot(addMinutes(snapshot.createdAt, 5), 2, 2);

  await updatePlayTimes();
  await checkPlayTimes([5 * 60, 5 * 60], [5 * 60, 5 * 60], [5 * 60, 5 * 60], 2 * 5 * 60);
});
