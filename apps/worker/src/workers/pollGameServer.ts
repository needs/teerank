import { prisma } from "../prisma";
import { resetPackets, getReceivedPackets, sendData, setupSockets, listenForPackets } from "../socket";
import { GameServerInfoPacket, unpackGameServerInfoPackets } from "../packets/gameServerInfo";
import { scheduleUpdatePlayTime, scheduleRankPlayer, PollGameServerJobData, processPollGameServerJobs, wait } from "@teerank/teerank";
import { GameServer, GameServerState, Prisma } from "@prisma/client";
import { getEnvInt } from "@teerank/teerank";
import { uniqBy } from "lodash";

function stringToCharCode(str: string) {
  return str.split('').map((char) => char.charCodeAt(0));
}

const PACKET_HEADER = Buffer.from([
  ...stringToCharCode('xe'),
  0xff,
  0xff,
  0xff,
  0xff,
]);

const PACKET_GETINFO = Buffer.from([
  ...PACKET_HEADER,
  0xff,
  0xff,
  0xff,
  0xff,
  ...stringToCharCode('gie3'),
  0
]);

const PACKET_GETINFO64 = Buffer.from([
  ...PACKET_HEADER,
  0xff,
  0xff,
  0xff,
  0xff,
  ...stringToCharCode('fstd'),
  0
]);

const MAX_FAILURE_COUNT = getEnvInt('MAX_FAILURE_COUNT', 30);

function skipPolling(gameServer: GameServer & { gameServerState: GameServerState | null }) {
  if (gameServer.failureCount === 0) {
    return false;
  }

  return Math.random() >= (1.0 / Math.min(gameServer.failureCount, MAX_FAILURE_COUNT));
}

export async function processGameServerInfo(
  gameServer: GameServer & { gameServerState: GameServerState | null },
  gameServerInfo: GameServerInfoPacket,
) {
  const map = await prisma.map.upsert({
    select: {
      id: true,
    },
    where: {
      name_gameTypeName: {
        name: gameServerInfo.map,
        gameTypeName: gameServerInfo.gameType,
      },
    },
    update: {},
    create: {
      name: gameServerInfo.map,
      gameType: {
        connectOrCreate: {
          create: {
            name: gameServerInfo.gameType,
          },
          where: {
            name: gameServerInfo.gameType,
          },
        },
      },
    },
  });

  // Multi-row writes are sorted by their conflict key: with concurrent
  // pollers sharing overlapping player sets, unordered multi-row upserts
  // deadlock.
  const uniqClans = [...new Set(gameServerInfo.clients.map((client) => client.clan).filter((clan) => clan !== ''))].sort();

  await prisma.clan.createMany({
    data: uniqClans.map((clan) => ({
      name: clan,
    })),
    skipDuplicates: true,
  });

  const uniqClients = uniqBy(gameServerInfo.clients, 'name')
    .sort((a, b) => (a.name < b.name ? -1 : a.name > b.name ? 1 : 0));

  if (uniqClients.length > 0) {
    const now = new Date();

    // createdAt/updatedAt are Prisma-managed and have to be set explicitly in
    // raw SQL. The DO UPDATE is a no-op unless lastSeenAt moved by more than
    // 10 minutes or the clan changed — the UI can't tell the difference, and
    // it saves up to 4 index writes per player. An empty clan means "no clan
    // info": it never overwrites a stored clan.
    await prisma.$executeRaw`
      INSERT INTO "Player" ("name", "clanName", "lastSeenAt", "createdAt", "updatedAt")
      VALUES ${Prisma.join(uniqClients.map((client) => Prisma.sql`(${client.name}, ${client.clan === "" ? null : client.clan}, ${now}, ${now}, ${now})`))}
      ON CONFLICT ("name") DO UPDATE SET
        "clanName" = COALESCE(EXCLUDED."clanName", "Player"."clanName"),
        "lastSeenAt" = EXCLUDED."lastSeenAt",
        "updatedAt" = EXCLUDED."updatedAt"
      WHERE EXCLUDED."lastSeenAt" - "Player"."lastSeenAt" > interval '10 minutes'
        OR (EXCLUDED."clanName" IS NOT NULL AND EXCLUDED."clanName" IS DISTINCT FROM "Player"."clanName")
    `;
  }

  const snapshot = await prisma.gameServerSnapshot.create({
    select: {
      id: true,
      clients: {
        select: {
          playerName: true,
          id: true
        }
      }
    },
    data: {
      gameServer: {
        connect: {
          id: gameServer.id,
        },
      },

      version: gameServerInfo.version,
      name: gameServerInfo.name,

      map: {
        connect: {
          id: map.id,
        },
      },
      numPlayers: gameServerInfo.numPlayers,
      maxPlayers: gameServerInfo.maxPlayers,
      numClients: gameServerInfo.numClients,
      maxClients: gameServerInfo.maxClients,

      clients: {
        createMany: {
          data: gameServerInfo.clients.map((client) => ({
            playerName: client.name,
            clanName: client.clan === "" ? undefined : client.clan,
            country: client.country,
            score: client.score,
            inGame: client.inGame,
          })),
        },
      }
    },
  });

  await prisma.gameServer.update({
    where: {
      id: gameServer.id,
    },
    data: {
      lastSeenAt: new Date(),
      failureCount: 0,
    },
  });

  const stateClients = gameServerInfo.clients.map((client) => ({
    playerName: client.name,
    clanName: client.clan === "" ? undefined : client.clan,
    country: client.country,
    score: client.score,
    inGame: client.inGame,
  }));

  await prisma.gameServerState.upsert({
    select: {
      id: true,
    },
    where: {
      gameServerId: gameServer.id,
    },
    update: {
      version: gameServerInfo.version,
      name: gameServerInfo.name,
      mapId: map.id,
      numPlayers: gameServerInfo.numPlayers,
      maxPlayers: gameServerInfo.maxPlayers,
      numClients: gameServerInfo.numClients,
      maxClients: gameServerInfo.maxClients,

      clients: {
        deleteMany: {},
        createMany: {
          data: stateClients,
        },
      },
    },
    create: {
      gameServerId: gameServer.id,
      version: gameServerInfo.version,
      name: gameServerInfo.name,
      mapId: map.id,
      numPlayers: gameServerInfo.numPlayers,
      maxPlayers: gameServerInfo.maxPlayers,
      numClients: gameServerInfo.numClients,
      maxClients: gameServerInfo.maxClients,

      clients: {
        createMany: {
          data: stateClients,
        },
      },
    },
  });

  return snapshot.id;
}

async function getGameServer(jobData: PollGameServerJobData) {
  if (jobData.gameServerId) {
    return await prisma.gameServer.findUniqueOrThrow({
      where: {
        id: jobData.gameServerId,
      },
      include: {
        gameServerState: true,
      },
    });
  } else if (jobData.ip && jobData.port) {
    return await prisma.gameServer.findUniqueOrThrow({
      where: {
        ip_port: {
          ip: jobData.ip,
          port: jobData.port,
        }
      },
      include: {
        gameServerState: true,
      },
    });
  }

  throw new Error(`Game server not found: ${jobData.ip}:${jobData.port} (${jobData.gameServerId})`);
}

async function processor(jobData: PollGameServerJobData) {
  const gameServer = await getGameServer(jobData);

  if (skipPolling(gameServer)) {
    return;
  }

  const sockets = await setupSockets();

  listenForPackets(sockets, gameServer.ip, gameServer.port);

  sendData(sockets, PACKET_GETINFO, gameServer.ip, gameServer.port);
  sendData(sockets, PACKET_GETINFO64, gameServer.ip, gameServer.port);

  await wait(2000);

  const receivedPackets = getReceivedPackets(sockets, gameServer.ip, gameServer.port);

  if (receivedPackets.packets.length > 0) {
    try {
      const gameServerInfo = unpackGameServerInfoPackets(receivedPackets.packets)
      const snapshotId = await processGameServerInfo(gameServer, gameServerInfo);

      await Promise.all([
        scheduleUpdatePlayTime({ snapshotId }),
        scheduleRankPlayer({ snapshotId })
      ]);
    } catch (e) {
      console.warn(`${gameServer.ip}:${gameServer.port}: ${e}`)
    }
  } else {
    await prisma.gameServerState.deleteMany({
      where: {
        gameServerId: gameServer.id,
      },
    });

    await prisma.gameServer.update({
      where: {
        id: gameServer.id,
      },
      data: {
        failureCount: {
          increment: 1,
        },
      },
    });
  }

  resetPackets(sockets, gameServer.ip, gameServer.port);
}

export async function startPollGameServerWorker() {
  return processPollGameServerJobs(processor);
}
