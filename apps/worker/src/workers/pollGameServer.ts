import { prisma } from "../prisma";
import { resetPackets, getReceivedPackets, sendData, setupSockets, listenForPackets } from "../socket";
import { GameServerInfoPacket, unpackGameServerInfoPackets } from "../packets/gameServerInfo";
import { scheduleUpdatePlayTime, scheduleRankPlayer, PollGameServerJobData, processPollGameServerJobs, wait } from "@teerank/teerank";
import { GameServer, GameServerState } from "@prisma/client";
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

  const uniqClans = [...new Set(gameServerInfo.clients.map((client) => client.clan).filter((clan) => clan !== ''))];

  await prisma.clan.createMany({
    data: uniqClans.map((clan) => ({
      name: clan,
    })),
    skipDuplicates: true,
  });

  const uniqClients = uniqBy(gameServerInfo.clients, 'name');

  for (const client of uniqClients) {
    await prisma.player.upsert({
      where: {
        name: client.name,
      },
      update: {
        clanName: client.clan === "" ? undefined : client.clan,
        lastSeenAt: new Date()
      },
      create: {
        name: client.name,
        clanName: client.clan === "" ? undefined : client.clan,
        lastSeenAt: new Date(),
      },
    });
  }

  for (const client of uniqClients) {
    await prisma.playerInfoGameType.upsert({
      select: {
        id: true,
      },
      where: {
        playerName_gameTypeName: {
          playerName: client.name,
          gameTypeName: gameServerInfo.gameType,
        },
      },
      update: {},
      create: {
        playerName: client.name,
        gameTypeName: gameServerInfo.gameType,
      },
    });
  }

  for (const clan of uniqClans) {
    await prisma.clanInfoGameType.upsert({
      select: {
        id: true,
      },
      where: {
        clanName_gameTypeName: {
          clanName: clan,
          gameTypeName: gameServerInfo.gameType,
        },
      },
      update: {},
      create: {
        clanName: clan,
        gameTypeName: gameServerInfo.gameType,
      },
    });
  }

  for (const client of uniqClients) {
    await prisma.playerInfoMap.upsert({
      select: {
        id: true,
      },
      where: {
        playerName_mapId: {
          playerName: client.name,
          mapId: map.id,
        },
      },
      update: {},
      create: {
        playerName: client.name,
        mapId: map.id,
      },
    });
  }

  for (const clan of uniqClans) {
    await prisma.clanInfoMap.upsert({
      select: {
        id: true,
      },
      where: {
        clanName_mapId: {
          clanName: clan,
          mapId: map.id,
        },
      },
      update: {},
      create: {
        clanName: clan,
        mapId: map.id,
      },
    });
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

      gameServerState: {
        create: {
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
      },
    },
  });

  // Delete any game server states that don't have a game server anymore.
  await prisma.gameServerState.deleteMany({
    where: {
      gameServerId: null,
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
