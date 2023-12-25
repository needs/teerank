import { RankMethod } from "@prisma/client";
import { clearDatabase } from "../../testSetup";
import { prisma } from "../prisma";
import { rankPlayers } from "./rankPlayers";

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

test('Already scheduled snapshot', async () => {
  const snapshot = await createSnapshot();
  const rankingStartedAt = new Date();

  await prisma.gameServerSnapshot.update({
    where: {
      id: snapshot.id,
    },
    data: {
      rankingStartedAt
    },
  });

  await rankPlayers();

  const snapshotAfter = await prisma.gameServerSnapshot.findUnique({
    where: {
      id: snapshot.id,
    },
    select: {
      rankedAt: true,
      rankingStartedAt: true,
    },
  });

  expect(snapshotAfter).toEqual({
    rankedAt: null,
    rankingStartedAt
  });
});

test('rankedAt', async () => {
  const snapshot = await createSnapshot();

  await rankPlayers();

  const snapshotAfter = await prisma.gameServerSnapshot.findUniqueOrThrow({
    select: {
      rankedAt: true,
      rankingStartedAt: true,
    },
    where: {
      id: snapshot.id,
    },
  });

  expect(snapshotAfter.rankingStartedAt).not.toBeNull();
  expect(snapshotAfter.rankedAt).not.toBeNull();
});
