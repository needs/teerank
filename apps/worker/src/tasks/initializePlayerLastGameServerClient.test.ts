import { clearDatabase, runJobNTimes } from "../../testSetup";
import { prisma } from "../prisma";
import { initializePlayerLastGameServerClient } from "./initializePlayerLastGameServerClient";

beforeEach(async () => {
  await clearDatabase();
});

async function createSnapshot(playerNames: string[]) {
  return await prisma.gameServerSnapshot.create({
    select: {
      clients: {
        select: {
          id: true,
        }
      }
    },
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
              },
            },
          },
        },
      },

      maxClients: playerNames.length,
      numClients: playerNames.length,
      maxPlayers: playerNames.length,
      numPlayers: playerNames.length,

      clients: {
        createMany: {
          data: playerNames.map((playerName) => ({
            playerName,
            score: 0,
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

test('Default values', async () => {
  const player = await prisma.player.create({
    data: {
      name: 'player',
    },
    select: {
      lastGameServerClient: true,
    }
  });

  expect(player.lastGameServerClient).toBeNull();
});

test('Already initialized', async () => {
  const player = await prisma.player.create({
    data: {
      name: 'player',
    },
    select: {
      name: true,
    }
  });

  const snapshot = await createSnapshot([player.name]);

  const playerBefore = await prisma.player.update({
    where: {
      name: player.name,
    },
    select: {
      lastGameServerClient: true,
    },
    data: {
      lastGameServerClient: {
        connect: {
          id: snapshot.clients[0].id,
        }
      }
    }
  });

  expect(playerBefore.lastGameServerClient).not.toBeNull();

  await runJobNTimes(1, initializePlayerLastGameServerClient);

  const playerAfter = await prisma.player.findUniqueOrThrow({
    where: {
      name: player.name,
    },
    select: {
      lastGameServerClient: true,
    }
  });

  expect(playerAfter.lastGameServerClient).toStrictEqual(playerBefore.lastGameServerClient);
});

test('No client', async () => {
  const player = await prisma.player.create({
    data: {
      name: 'player',
    },
    select: {
      name: true,
    }
  });

  await runJobNTimes(1, initializePlayerLastGameServerClient);

  const playerAfter = await prisma.player.findUniqueOrThrow({
    where: {
      name: player.name,
    },
    select: {
      lastGameServerClient: true,
    }
  });

  expect(playerAfter.lastGameServerClient).toBeNull();
});

test('Has client', async () => {
  const uninitializedPlayer = await prisma.player.create({
    data: {
      name: 'player',
    },
    select: {
      name: true,
    }
  });

  const snapshot = await createSnapshot([uninitializedPlayer.name]);

  await runJobNTimes(2, initializePlayerLastGameServerClient);

  const initializedPlayer = await prisma.player.findUniqueOrThrow({
    where: {
      name: uninitializedPlayer.name,
    },
    select: {
      lastGameServerClient: true,
    }
  });

  expect(initializedPlayer.lastGameServerClient?.id).toBe(snapshot.clients[0].id);
});
