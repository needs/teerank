import { PrismaClient } from "@prisma/client";
import { max } from "lodash";
import Fuse, { FuseResult } from "fuse.js";
import { IndexedPlayer, IndexedClan, randomRange, wait, IndexedGameServer, indexGameServerSchema, indexClanSchema, indexPlayerSchema } from "@teerank/teerank";
import { minutesToMilliseconds } from "date-fns";
import { captureException } from "@sentry/node";
import { readFileSync, writeFileSync } from "fs";
import { z } from "zod";

const DUMP_VERSION = 1;
const DUMP_PATH = process.env.DUMP_PATH ?? 'search-index.json';

const prisma = new PrismaClient();

const indexedPlayers: Record<string, IndexedPlayer> = {};
const fusePlayers = new Fuse(Object.values(indexedPlayers), { keys: ['name'], includeScore: true });

const indexedClans: Record<string, IndexedClan> = {};
const fuseClans = new Fuse(Object.values(indexedClans), { keys: ['name'], includeScore: true });

const indexedGameServers: Record<string, IndexedGameServer> = {};
const fuseGameServers = new Fuse(Object.values(indexedGameServers), { keys: ['name'], includeScore: true });

enum IndexStatus {
  IN_PROGRESS,
  COMPLETED,
}

async function updatePlayers() {
  const playersUpdatedAt = max(Object.values(indexedPlayers).map(player => player.updatedAt)) ?? new Date(0);

  const players = await prisma.player.findMany({
    where: {
      updatedAt: {
        gt: playersUpdatedAt,
      },
    },
    include: {
      gameServerStateClients: {
        select: {
          gameServerState: {
            select: {
              gameServer: true,
            },
          },
        },
      },
    },
    orderBy: {
      updatedAt: 'asc',
    },
    take: 100,
  });

  Object.assign(indexedPlayers, Object.fromEntries(players.map(player => ([player.name, {
    name: player.name,
    clanName: player.clanName,
    playTime: Number(player.playTime),
    updatedAt: player.updatedAt,
    lastSeenAt: player.lastSeenAt,
    gameServers: player.gameServerStateClients.map(client => ({
      ip: client.gameServerState.gameServer.ip,
      port: client.gameServerState.gameServer.port
    })),
  }]))))

  return players.length < 100 ? IndexStatus.COMPLETED : IndexStatus.IN_PROGRESS;
}

async function indexPlayers() {
  fusePlayers.setCollection(Object.values(indexedPlayers));
  console.log(`Indexed ${Object.keys(indexedPlayers).length} players`);
}

async function updateClans() {
  const clansUpdatedAt = max(Object.values(indexedClans).map(clan => clan.updatedAt)) ?? new Date(0);

  const clans = await prisma.clan.findMany({
    select: {
      name: true,
      playTime: true,
      updatedAt: true,
      _count: {
        select: {
          players: true,
        },
      },
    },
    where: {
      updatedAt: {
        gt: clansUpdatedAt,
      },
    },
    orderBy: {
      updatedAt: 'asc',
    },
    take: 100,
  });

  Object.assign(indexedClans, Object.fromEntries(clans.map(clan => ([clan.name, {
    ...clan,
    playTime: Number(clan.playTime),
    playerCount: clan._count.players,
  }]))))
  fuseClans.setCollection(Object.values(indexedClans));

  return clans.length < 100 ? IndexStatus.COMPLETED : IndexStatus.IN_PROGRESS;
}

async function indexClans() {
  fuseClans.setCollection(Object.values(indexedClans));
  console.log(`Indexed ${Object.keys(indexedClans).length} clans`);
}

async function updateGameServers() {
  const gameServersUpdatedAt = max(Object.values(indexedGameServers).map(gameServer => gameServer.updatedAt)) ?? new Date(0);

  const gameServers = await prisma.gameServerState.findMany({
    select: {
      gameServer: {
        select: {
          ip: true,
          port: true,
        },
      },
      numClients: true,
      maxClients: true,
      updatedAt: true,
      name: true,
      map: {
        select: {
          name: true,
          gameTypeName: true,
        },
      },
    },
    where: {
      updatedAt: {
        gt: gameServersUpdatedAt,
      },
    },
  });

  Object.assign(indexedGameServers, Object.fromEntries(gameServers.map(gameServer => ([gameServer.name, {
    ip: gameServer.gameServer.ip,
    port: gameServer.gameServer.port,
    name: gameServer.name,
    gameType: gameServer.map.gameTypeName,
    map: gameServer.map.name,
    clientCount: gameServer.numClients,
    clientMax: gameServer.maxClients,
    updatedAt: gameServer.updatedAt,
  }]))))

  return gameServers.length < 100 ? IndexStatus.COMPLETED : IndexStatus.IN_PROGRESS;
}

async function indexGameServers() {
  fuseGameServers.setCollection(Object.values(indexedGameServers));
  console.log(`Indexed ${Object.keys(indexedGameServers).length} game servers`);
}

function processResults<T>(results: FuseResult<T>[], sortBy: (item: T) => number) {
  return results.filter(result => result.score < 0.7).sort((a, b) => a.score - b.score || sortBy(b.item) - sortBy(a.item)).map(result => result.item);
}

export function searchPlayers(query: string) {
  return processResults(fusePlayers.search(query, { limit: 30 }), player => player.playTime);
}

export function searchClans(query: string) {
  return processResults(fuseClans.search(query, { limit: 30 }), clan => clan.playTime);
}

export function searchGameServers(query: string) {
  return processResults(fuseGameServers.search(query, { limit: 30 }), gameServer => gameServer.clientCount);
}

const dumpSchema = z.object({
  version: z.number(),
  players: z.record(indexPlayerSchema),
  clans: z.record(indexClanSchema),
  gameServers: z.record(indexGameServerSchema),
}).required();

type Dump = z.infer<typeof dumpSchema>;

async function dump() {
  const data = {
    version: DUMP_VERSION,
    players: indexedPlayers,
    clans: indexedClans,
    gameServers: indexedGameServers
  } satisfies Dump;

  writeFileSync(DUMP_PATH, JSON.stringify(data, null, 2));

  console.log(`Dumped ${Object.keys(indexedPlayers).length} players, ${Object.keys(indexedClans).length} clans, ${Object.keys(indexedGameServers).length} game servers`);
}

async function restore() {
  try {
    const data = dumpSchema.parse(JSON.parse(readFileSync(DUMP_PATH, 'utf8')));

    if (data.version !== DUMP_VERSION) {
      console.log(`Invalid dump version ${data.version}, expected ${DUMP_VERSION}`);
      return;
    }

    Object.assign(indexedPlayers, data.players);
    Object.assign(indexedClans, data.clans);
    Object.assign(indexedGameServers, data.gameServers);
    console.log(`Restored from dump, indexed ${Object.keys(indexedPlayers).length} players, ${Object.keys(indexedClans).length} clans, ${Object.keys(indexedGameServers).length} game servers`);

  } catch (error) {
    if (error instanceof Error && 'code' in error && error.code === 'ENOENT') {
      console.log('No dump file found, starting fresh');
    } else {
      console.error("Failed to restore from dump", error);
      captureException(error);
    }
  }
}

async function runInBackground(update: () => Promise<IndexStatus>, index: () => Promise<void>) {
  for (; ;) {
    const status = await update().catch(error => {
      console.error("updating failed", error);
      captureException(error);
      return IndexStatus.IN_PROGRESS;
    });

    if (status === IndexStatus.COMPLETED) {
      await index().catch(error => {
        console.error("indexing failed", error);
        captureException(error);
      });

      const spread = randomRange(minutesToMilliseconds(4), minutesToMilliseconds(6));
      await wait(spread);
    } else {
      // Don't overload the database
      await wait(50);
    }
  }
}

export async function startBackgroundIndexing() {
  await restore();

  runInBackground(updatePlayers, indexPlayers);
  runInBackground(updateClans, indexClans);
  runInBackground(updateGameServers, indexGameServers);

  setInterval(dump, minutesToMilliseconds(10));
}
