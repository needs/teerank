import { TaskRunStatus } from "@prisma/client";
import { clearDatabase } from "../../testSetup";
import { prisma } from "../prisma";
import { fillClanAndPlayerInfos } from "./fillClanAndPlayerInfos";

beforeEach(async () => {
  await clearDatabase();
});

async function createClient({ withClan }: { withClan?: boolean }) {
  const player = await prisma.player.upsert({
    where: {
      name: "player",
    },
    update: {},
    create: {
      name: "player",
    },
  });

  if (withClan) {
    await prisma.clan.upsert({
      where: {
        name: "clan",
      },
      update: {},
      create: {
        name: "clan",
      },
    });
  }

  const gameType = await prisma.gameType.upsert({
    where: {
      name: "gameType",
    },
    update: {},
    create: {
      name: "gameType",
    },
  });

  const map = await prisma.map.upsert({
    where: {
      name_gameTypeName: {
        name: "map",
        gameTypeName: gameType.name,
      },
    },
    update: {},
    create: {
      name: "map",
      gameType: {
        connect: {
          name: gameType.name,
        },
      },
    },
  });

  const gameServer = await prisma.gameServer.upsert({
    where: {
      ip_port: {
        ip: "ip",
        port: 0,
      },
    },
    update: {},
    create: {
      ip: "ip",
      port: 0,
    },
  });

  const gameServerSnapshot = await prisma.gameServerSnapshot.create({
    data: {
      createdAt: new Date(),
      map: {
        connect: {
          id: map.id,
        },
      },
      gameServer: {
        connect: {
          id: gameServer.id,
        },
      },
      gameServerLast: {
        connect: {
          id: gameServer.id,
        },
      },
      version: "version",
      name: "name",
      numPlayers: 0,
      maxPlayers: 0,
      numClients: 0,
      maxClients: 0,
    }
  });

  const gameServerClient = await prisma.gameServerClient.create({
    data: {
      country: 0,
      inGame: true,
      score: 0,
      player: {
        connect: {
          name: player.name,
        }
      },
      snapshot: {
        connect: {
          id: gameServerSnapshot.id,
        }
      },
      clan: {
        connect: withClan ? {
          name: "clan",
        } : undefined,
      },
    },
  });

  return () =>
    prisma.gameServerClient.findUniqueOrThrow({
      where: {
        id: gameServerClient.id,
      },
    });
}

test('Single player without clan', async () => {
  const getClient = await createClient({ withClan: false });
  const status = await fillClanAndPlayerInfos();
  const client = await getClient();

  expect(status).toBe(TaskRunStatus.COMPLETED);

  expect(client.playerInfoGameTypeId).not.toBeNull();
  expect(client.playerInfoMapId).not.toBeNull();

  expect(client.clanInfoGameTypeId).toBeNull();
  expect(client.clanInfoMapId).toBeNull();
});

test('Single player with clan', async () => {
  const getClient = await createClient({ withClan: true });
  const status = await fillClanAndPlayerInfos();
  const client = await getClient();

  expect(status).toBe(TaskRunStatus.COMPLETED);

  expect(client.playerInfoGameTypeId).not.toBeNull();
  expect(client.playerInfoMapId).not.toBeNull();

  expect(client.clanInfoGameTypeId).not.toBeNull();
  expect(client.clanInfoMapId).not.toBeNull();
});
