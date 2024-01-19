import { RankMethod } from "@prisma/client";
import { clearDatabase, runJobNTimes } from "../../testSetup";
import { prisma } from "../prisma";
import { removeEmptySnapshots } from "./removeEmptySnapshots";
import { addMinutes, subMinutes } from "date-fns";

beforeEach(async () => {
  await clearDatabase();
});

async function createSnapshot(isEmpty = true) {
  const gameServer = await prisma.gameServer.upsert({
    where: {
      ip_port: {
        ip: 'localhost',
        port: 8303,
      },
    },
    update: {},
    create: {
      ip: 'localhost',
      port: 8303,
    }
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

      maxClients: 0,
      numClients: 0,
      maxPlayers: 0,
      numPlayers: 0,

      name: isEmpty ? '' : 'non-empty',
      version: isEmpty ? '' : 'non-empty',

      gameServer: {
        connect: {
          id: gameServer.id,
        }
      },

      gameServerLast: {
        connect: {
          id: gameServer.id,
        }
      },
    }
  });
}

test('Game server with only empty snapshots', async () => {
  const snapshot = await createSnapshot();

  await runJobNTimes(2, removeEmptySnapshots);

  const snapshotAfter = await prisma.gameServerSnapshot.findUnique({
    where: {
      id: snapshot.id,
    },
    select: {
      id: true
    },
  });

  const gameServerAfter = await prisma.gameServer.findUniqueOrThrow({
    where: {
      id: snapshot.gameServerId,
    },
    select: {
      offlineSince: true,
    },
  });

  expect(snapshotAfter).toBeNull();
  expect(gameServerAfter.offlineSince).toStrictEqual(snapshot.createdAt);
});

test('Game server with no snapshots', async () => {
  const gameServer = await prisma.gameServer.create({
    data: {
      ip: 'localhost',
      port: 8303,
    }
  });

  await runJobNTimes(1, removeEmptySnapshots);

  const gameServerAfter = await prisma.gameServer.findUniqueOrThrow({
    where: {
      id: gameServer.id,
    },
    select: {
      offlineSince: true,
    },
  });

  expect(gameServerAfter.offlineSince).toBeNull();
});

test('Game server with non-empty snapshot followed by empty snapshot', async () => {
  const nonEmptySnapshot = await createSnapshot(false);
  const emptySnapshot = await createSnapshot();

  await runJobNTimes(2, removeEmptySnapshots);

  const snapshotsAfter = await prisma.gameServerSnapshot.findMany({
    where: {
      id: {
        in: [nonEmptySnapshot.id, emptySnapshot.id],
      }
    },
    select: {
      id: true,
    },
  });

  const gameServerAfter = await prisma.gameServer.findUniqueOrThrow({
    where: {
      id: nonEmptySnapshot.gameServerId,
    },
    select: {
      offlineSince: true,
    },
  });

  expect(snapshotsAfter.map(snapshot => snapshot.id)).toStrictEqual([nonEmptySnapshot.id]);
  expect(gameServerAfter.offlineSince).toStrictEqual(nonEmptySnapshot.createdAt);
});

test('Game server with empty snapshot followed by non-empty snapshot', async () => {
  const emptySnapshot = await createSnapshot();
  const nonEmptySnapshot = await createSnapshot(false);

  await runJobNTimes(2, removeEmptySnapshots);

  const snapshotsAfter = await prisma.gameServerSnapshot.findMany({
    where: {
      id: {
        in: [nonEmptySnapshot.id, emptySnapshot.id],
      }
    },
    select: {
      id: true,
    },
  });

  const gameServerAfter = await prisma.gameServer.findUniqueOrThrow({
    where: {
      id: nonEmptySnapshot.gameServerId,
    },
    select: {
      offlineSince: true,
    },
  });

  expect(snapshotsAfter.map(snapshot => snapshot.id)).toStrictEqual([nonEmptySnapshot.id]);
  expect(gameServerAfter.offlineSince).toBeNull();
});

test('Game server with offlineSince set to a later date', async () => {
  const emptySnapshot = await createSnapshot();

  await prisma.gameServer.update({
    where: {
      id: emptySnapshot.gameServerId,
    },
    data: {
      offlineSince: addMinutes(emptySnapshot.createdAt, 5),
    },
  });

  await runJobNTimes(2, removeEmptySnapshots);

  const gameServerAfter = await prisma.gameServer.findUniqueOrThrow({
    where: {
      id: emptySnapshot.gameServerId,
    },
    select: {
      offlineSince: true,
    },
  });

  expect(gameServerAfter.offlineSince).toStrictEqual(emptySnapshot.createdAt);
});

test('Game server with offlineSince set to a earlier date', async () => {
  const emptySnapshot = await createSnapshot();

  const { offlineSince } = await prisma.gameServer.update({
    where: {
      id: emptySnapshot.gameServerId,
    },
    data: {
      offlineSince: subMinutes(emptySnapshot.createdAt, 5),
    },
    select: {
      offlineSince: true,
    }
  });

  await runJobNTimes(2, removeEmptySnapshots);

  const gameServerAfter = await prisma.gameServer.findUniqueOrThrow({
    where: {
      id: emptySnapshot.gameServerId,
    },
    select: {
      offlineSince: true,
    },
  });

  expect(gameServerAfter.offlineSince).toStrictEqual(offlineSince);
});
