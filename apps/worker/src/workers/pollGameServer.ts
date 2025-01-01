import { prisma } from "../prisma";
import { resetPackets, getReceivedPackets, sendData, setupSockets, listenForPackets } from "../socket";
import { GameServerInfoPacket, unpackGameServerInfoPackets } from "../packets/gameServerInfo";
import { getQueueRankPlayer, getQueueUpdatePlayTime, QUEUE_NAME_POLL_GAME_SERVER, wait } from "@teerank/teerank";
import { GameServer, GameServerState } from "@prisma/client";
import { Job, Worker } from "bullmq";
import { bullmqConnection } from "@teerank/teerank";
import { getEnvInt } from "@teerank/teerank";
import { uniqBy } from "lodash";
import { minutesToSeconds } from "date-fns";

const POLL_GAME_SERVER_CONCURRENCY = getEnvInt('POLL_GAME_SERVER_CONCURRENCY', 100);

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

const queueUpdatePlayTime = getQueueUpdatePlayTime();
const queueRankPlayer = getQueueRankPlayer();

export type OnUpdatePlayerClanHook = ((playerName: string, oldClanName: string | null, newClanName: string | null) => Promise<void>);

async function changePlayerClans(
  playerClans: Record<string, string | null>,
  onUpdate: OnUpdatePlayerClanHook | undefined
) {
  const totalPlayerCount = Object.keys(playerClans).length;
  const clanDelta: Record<string, number> = {};

  // When changing clans, clan `activePlayerCount` needs to be updated for both
  // the old and new clans.  To avoid race conditions, when updating a player's
  // clan, make sure old player clan is accurate.

  for (let i = 0; i < 10; i++) {
    await Promise.all(Object.entries(playerClans).map(async ([playerName, newClanName_]) => {
      const newClanName = newClanName_ || null;

      const currentPlayer = await prisma.player.findUnique({
        where: { name: playerName },
        select: { clanName: true },
      });

      const oldClanName = currentPlayer?.clanName || null;

      // Used by tests to simulate race conditions.
      if (onUpdate !== undefined) {
        await onUpdate(playerName, oldClanName, newClanName);
      }

      const ret = await prisma.player.updateMany({
        where: { name: playerName, clanName: oldClanName },
        data: { clanName: newClanName },
      });

      if (ret.count !== 0) {
        if (oldClanName !== null) {
          clanDelta[oldClanName] = (clanDelta[oldClanName] ?? 0) - 1;
        }

        if (newClanName !== null) {
          clanDelta[newClanName] = (clanDelta[newClanName] ?? 0) + 1;
        }

        delete playerClans[playerName];
      }
    }));

    const remainingPlayerCount = Object.keys(playerClans).length;
    if (remainingPlayerCount > 0) {
      console.log(`${remainingPlayerCount}/${totalPlayerCount} players left to update (${i})`);
    } else {
      break;
    }
  }

  const remainingPlayerCount = Object.keys(playerClans).length;
  if (remainingPlayerCount > 0) {
    console.error(`${remainingPlayerCount}/${totalPlayerCount} players failed to update`);
  }

  await Promise.all(Object.entries(clanDelta).map(async ([clanName, delta]) => {
    if (delta !== 0) {
      await prisma.clan.update({
        where: { name: clanName },
        data: { activePlayerCount: { increment: delta } },
      });
    }
  }));
}

export async function processGameServerInfo(
  gameServer: GameServer & { gameServerState: GameServerState | null },
  gameServerInfo: GameServerInfoPacket,
  onUpdatePlayerClanHook: OnUpdatePlayerClanHook | undefined = undefined
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

  await prisma.player.createMany({
    data: uniqClients.map((client) => ({
      name: client.name,
    })),
    skipDuplicates: true,
  });

  await changePlayerClans(
    Object.fromEntries(uniqClients.map((client) => [client.name, client.clan])),
    onUpdatePlayerClanHook
  );

  await Promise.all(uniqClients.map((client) => prisma.playerInfoGameType.upsert({
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
  })));

  await Promise.all(uniqClans.map((clan) => prisma.clanInfoGameType.upsert({
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
  })));

  await Promise.all(uniqClients.map((client) => prisma.playerInfoMap.upsert({
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
  })));

  await Promise.all(uniqClans.map((clan) => prisma.clanInfoMap.upsert({
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
  })));

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

  await prisma.$transaction(async (tx) => {
    // I couldn't find an elegant way to do replace all clients for an
    // existing game server state so we delete the existing one and create
    // a new one.  Do it in a transaction to avoid race conditions.

    await tx.gameServerState.deleteMany({
      where: {
        gameServerId: gameServer.id,
      },
    });

    await tx.gameServerState.create({
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
  });

  await Promise.all(
    snapshot.clients.map((client) =>
      prisma.player.update({
        where: {
          name: client.playerName,
        },
        data: {
          lastSeenAt: new Date(),
        },
      })
    )
  );

  await prisma.gameServer.update({
    where: {
      id: gameServer.id,
    },
    data: {
      lastSeenAt: new Date(),
      failureCount: 0,
    },
  });

  return snapshot.id;
}

async function processor(job: Job) {
  const gameServer =
    await prisma.gameServer.findUniqueOrThrow({
      where: {
        ip_port: {
          ip: job.data.ip,
          port: job.data.port,
        }
      },
      include: {
        gameServerState: true,
      },
    });


  if (skipPolling(gameServer)) {
    return;
  }

  const sockets = await setupSockets;

  listenForPackets(sockets, gameServer.ip, gameServer.port);

  sendData(sockets, PACKET_GETINFO, gameServer.ip, gameServer.port);
  sendData(sockets, PACKET_GETINFO64, gameServer.ip, gameServer.port);

  await wait(2000);

  const receivedPackets = getReceivedPackets(sockets, gameServer.ip, gameServer.port);

  if (receivedPackets.packets.length > 0) {
    try {
      const gameServerInfo = unpackGameServerInfoPackets(receivedPackets.packets)
      const snapshotId = await processGameServerInfo(gameServer, gameServerInfo);

      const jobName = `${gameServer.ip} - ${gameServer.port} - ${snapshotId}`;

      await Promise.all([
        queueUpdatePlayTime.add(jobName, {
          snapshotId: snapshotId,
        }, {
          removeOnComplete: 1000,
          removeOnFail: 1000,
        }),

        queueRankPlayer.add(jobName, {
          snapshotId: snapshotId,
        }, {
          removeOnComplete: 1000,
          removeOnFail: 1000,
        })
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
  return new Worker(QUEUE_NAME_POLL_GAME_SERVER, processor, {
    connection: bullmqConnection,
    concurrency: POLL_GAME_SERVER_CONCURRENCY,
    removeOnComplete: {
      age: minutesToSeconds(10),
    },
    removeOnFail: {
      count: 1000,
    }
  });
}
