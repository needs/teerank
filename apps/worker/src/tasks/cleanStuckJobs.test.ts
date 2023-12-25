import { subMinutes } from "date-fns";
import { clearDatabase } from "../../testSetup";
import { prisma } from "../prisma";
import { cleanStuckJobs, jobTimeoutMinutes } from "./cleanStuckJobs";

beforeEach(async () => {
  await clearDatabase();
});

async function createSnapshot() {
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

test('Stuck master server polling job', async () => {
  const masterServer = await prisma.masterServer.create({
    data: {
      address: 'localhost',
      pollingStartedAt: subMinutes(new Date(), jobTimeoutMinutes + 1),
      port: 8303,
    }
  });

  await cleanStuckJobs();

  const updatedMasterServer = await prisma.masterServer.findUniqueOrThrow({
    where: {
      id: masterServer.id,
    },
  });

  expect(updatedMasterServer.pollingStartedAt).toBeNull();
})

test('Running master server polling job', async () => {
  const masterServer = await prisma.masterServer.create({
    data: {
      address: 'localhost',
      pollingStartedAt: new Date(),
      port: 8303,
    }
  });

  await cleanStuckJobs();

  const updatedMasterServer = await prisma.masterServer.findUniqueOrThrow({
    where: {
      id: masterServer.id,
    },
  });

  expect(updatedMasterServer.pollingStartedAt).not.toBeNull();
});

test('Stuck game server polling job', async () => {
  const gameServer = await prisma.gameServer.create({
    data: {
      ip: 'localhost',
      pollingStartedAt: subMinutes(new Date(), jobTimeoutMinutes + 1),
      port: 8303,
    }
  });

  await cleanStuckJobs();

  const updatedGameServer = await prisma.gameServer.findUniqueOrThrow({
    where: {
      id: gameServer.id,
    },
  });

  expect(updatedGameServer.pollingStartedAt).toBeNull();
})

test('Running game server polling job', async () => {
  const gameServer = await prisma.gameServer.create({
    data: {
      ip: 'localhost',
      pollingStartedAt: new Date(),
      port: 8303,
    }
  });

  await cleanStuckJobs();

  const updatedGameServer = await prisma.gameServer.findUniqueOrThrow({
    where: {
      id: gameServer.id,
    },
  });

  expect(updatedGameServer.pollingStartedAt).not.toBeNull();
});

test('Stuck snapshot ranking job', async () => {
  const snapshot = await createSnapshot();

  await prisma.gameServerSnapshot.update({
    where: {
      id: snapshot.id,
    },
    data: {
      rankingStartedAt: subMinutes(new Date(), jobTimeoutMinutes + 1),
    }
  });

  await cleanStuckJobs();

  const updatedSnapshot = await prisma.gameServerSnapshot.findUniqueOrThrow({
    where: {
      id: snapshot.id,
    },
  });

  expect(updatedSnapshot.rankingStartedAt).toBeNull();
})

test('Running snapshot ranking job', async () => {
  const snapshot = await createSnapshot();

  await prisma.gameServerSnapshot.update({
    where: {
      id: snapshot.id,
    },
    data: {
      rankingStartedAt: new Date(),
    }
  });

  await cleanStuckJobs();

  const updatedSnapshot = await prisma.gameServerSnapshot.findUniqueOrThrow({
    where: {
      id: snapshot.id,
    },
  });

  expect(updatedSnapshot.rankingStartedAt).not.toBeNull();
})

test('Stuck snapshot play timing job', async () => {
  const snapshot = await createSnapshot();

  await prisma.gameServerSnapshot.update({
    where: {
      id: snapshot.id,
    },
    data: {
      playTimingStartedAt: subMinutes(new Date(), jobTimeoutMinutes + 1),
    }
  });

  await cleanStuckJobs();

  const updatedSnapshot = await prisma.gameServerSnapshot.findUniqueOrThrow({
    where: {
      id: snapshot.id,
    },
  });

  expect(updatedSnapshot.playTimingStartedAt).toBeNull();
})

test('Running snapshot play timing job', async () => {
  const snapshot = await createSnapshot();

  await prisma.gameServerSnapshot.update({
    where: {
      id: snapshot.id,
    },
    data: {
      playTimingStartedAt: new Date(),
    }
  });

  await cleanStuckJobs();

  const updatedSnapshot = await prisma.gameServerSnapshot.findUniqueOrThrow({
    where: {
      id: snapshot.id,
    },
  });

  expect(updatedSnapshot.playTimingStartedAt).not.toBeNull();
})
