import { addMinutes } from "date-fns";
import { clearDatabase, runJobNTimes } from "../../testSetup";
import { prisma } from "../prisma";
import { updateGameServerPlayTimes } from "./updateGameServerPlayTimes";

beforeEach(async () => {
  await clearDatabase();
});

async function createSnapshot(createdAt: Date, numPlayers: number) {
  const playerIndices = [...Array(numPlayers).keys()];

  await prisma.player.createMany({
    data: playerIndices.map((index) => ({
      name: `player${index}`,
    })),

    skipDuplicates: true,
  });

  return await prisma.gameServerSnapshot.create({
    data: {
      createdAt,

      clients: {
        createMany: {
          data: playerIndices.map((index) => ({
            playerName: `player${index}`,
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

async function checkPlayTimes(expectedGameServerPlayTime: number) {
  const gameServerPlayTime = await prisma.gameServer.findUniqueOrThrow({
    where: {
      ip_port: {
        ip: 'localhost',
        port: 8303,
      }
    },
    select: {
      playTime: true,
    },
  });

  expect(gameServerPlayTime.playTime).toEqual(BigInt(expectedGameServerPlayTime));
}

test('Single snapshot', async () => {
  await createSnapshot(new Date(), 0);
  await runJobNTimes(2, updateGameServerPlayTimes);
  await checkPlayTimes(0);
});

test('One player', async () => {
  const snapshot1 = await createSnapshot(new Date(), 1);
  await createSnapshot(addMinutes(snapshot1.createdAt, 5), 1);

  await runJobNTimes(3, updateGameServerPlayTimes);
  await checkPlayTimes(5 * 60);
});

test('Many players', async () => {
  const snapshot1 = await createSnapshot(new Date(), 10);
  await createSnapshot(addMinutes(snapshot1.createdAt, 5), 10);

  await runJobNTimes(3, updateGameServerPlayTimes);
  await checkPlayTimes(5 * 60 * 10);
});
