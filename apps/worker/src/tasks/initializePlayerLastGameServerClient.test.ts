import { clearDatabase } from "../../testSetup";
import { prisma } from "../prisma";
import { initializePlayerLastGameServerClient } from "./initializePlayerLastGameServerClient";

beforeEach(async () => {
  await clearDatabase();
});

async function createSnapshot(createdAt: Date) {
  return await prisma.gameServerSnapshot.create({
    data: {
      createdAt,

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

      maxClients: 1,
      numClients: 1,
      maxPlayers: 1,
      numPlayers: 1,

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
      lastGameServerClientInitializedAt: true,
      lastGameServerClient: true,
    }
  });

  expect(player.lastGameServerClientInitializedAt).not.toBeNull();
  expect(player.lastGameServerClient).toBeNull();
});

test('Already initialized', async () => {
  const player = await prisma.player.create({
    data: {
      name: 'player',
    },
    select: {
      lastGameServerClientInitializedAt: true,
      lastGameServerClient: true,
    }
  });

  await initializePlayerLastGameServerClient(0, 0);

  expect(player.lastGameServerClientInitializedAt).not.toBeNull();
  expect(player.lastGameServerClient).toBeNull();
});

test('No client', async () => {
  const player = await prisma.player.create({
    data: {
      name: 'player',
      lastGameServerClientInitializedAt: null,
    },
    select: {
      lastGameServerClientInitializedAt: true,
      lastGameServerClient: true,
    }
  });

  await initializePlayerLastGameServerClient(0, 0);

  expect(player.lastGameServerClientInitializedAt).toBeNull();
  expect(player.lastGameServerClient).toBeNull();
});

test('Has client', async () => {
  const uninitializedPlayer = await prisma.player.create({
    data: {
      name: 'player',
      lastGameServerClientInitializedAt: null,
    },
    select: {
      name: true,
    }
  });

  const snapshot = await createSnapshot(new Date());

  const gameServerClient = await prisma.gameServerClient.create({
    data: {
      player: {
        connect: {
          name: uninitializedPlayer.name,
        },
      },
      score: 0,
      inGame: false,
      country: 0,
      snapshot: {
        connect: {
          id: snapshot.id,
        }
      },
    },
    select: {
      id: true,
    },
  });

  await initializePlayerLastGameServerClient(0, 0);

  const initializedPlayer = await prisma.player.findUniqueOrThrow({
    where: {
      name: uninitializedPlayer.name,
    },
    select: {
      lastGameServerClientInitializedAt: true,
      lastGameServerClient: true,
    }
  });

  expect(initializedPlayer.lastGameServerClientInitializedAt).not.toBeNull();
  expect(initializedPlayer.lastGameServerClient?.id).toBe(gameServerClient.id);
});
