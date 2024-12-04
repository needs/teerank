import { PrismaClient } from "@prisma/client";
import { max } from "lodash";
import Fuse, { FuseResult } from "fuse.js";
import { IndexedPlayer, IndexedClan, randomRange, wait, IndexedGameServer, indexGameServerSchema, indexClanSchema, indexPlayerSchema } from "@teerank/teerank";
import { minutesToMilliseconds } from "date-fns";
import { captureException } from "@sentry/node";
import { readFileSync, statSync, writeFileSync } from "fs";
import { z } from "zod";

const DUMP_VERSION = 1;
const DUMP_PATH = process.env.DUMP_PATH ?? 'search-index.json';

const prisma = new PrismaClient();

type FusePlayer = Pick<IndexedPlayer, 'name'>;
type FuseClan = Pick<IndexedClan, 'name'>;
type FuseGameServer = Pick<IndexedGameServer, 'name'>;

const indexedPlayers: Record<string, IndexedPlayer> = {};
const fusePlayers = new Fuse<FusePlayer>(Object.values({}), { keys: ['name'], includeScore: true });

const indexedClans: Record<string, IndexedClan> = {};
const fuseClans = new Fuse<FuseClan>(Object.values({}), { keys: ['name'], includeScore: true });

const indexedGameServers: Record<string, IndexedGameServer> = {};
const fuseGameServers = new Fuse<FuseGameServer>(Object.values({}), { keys: ['name'], includeScore: true });

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
  }).then(players => players.map(player => ({
    name: player.name,
    clanName: player.clanName,
    playTime: Number(player.playTime),
    updatedAt: player.updatedAt,
    lastSeenAt: player.lastSeenAt,
    gameServers: player.gameServerStateClients.map(client => ({
      ip: client.gameServerState.gameServer.ip,
      port: client.gameServerState.gameServer.port
    })),
  })));

  const newPlayers = players.filter(player => !(player.name in indexedPlayers));

  for (const player of newPlayers) {
    fusePlayers.add(player);
  }

  Object.assign(indexedPlayers, Object.fromEntries(players.map(player => ([player.name, player]))))

  return players.length < 100 ? IndexStatus.COMPLETED : IndexStatus.IN_PROGRESS;
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
  }).then(clans => clans.map(clan => ({
    ...clan,
    playTime: Number(clan.playTime),
    playerCount: clan._count.players,
  })));

  const newClans = clans.filter(clan => !(clan.name in indexedClans));

  for (const clan of newClans) {
    fuseClans.add(clan);
  }

  Object.assign(indexedClans, Object.fromEntries(clans.map(clan => ([clan.name, clan]))))

  return clans.length < 100 ? IndexStatus.COMPLETED : IndexStatus.IN_PROGRESS;
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
  }).then(gameServers => gameServers.map(gameServer => ({
    ip: gameServer.gameServer.ip,
    port: gameServer.gameServer.port,
    name: gameServer.name,
    gameType: gameServer.map.gameTypeName,
    map: gameServer.map.name,
    clientCount: gameServer.numClients,
    clientMax: gameServer.maxClients,
    updatedAt: gameServer.updatedAt,
  })));

  const newGameServers = gameServers.filter(gameServer => !(gameServer.name in indexedGameServers));

  for (const gameServer of newGameServers) {
    fuseGameServers.add(gameServer);
  }

  Object.assign(indexedGameServers, Object.fromEntries(gameServers.map(gameServer => ([gameServer.name, gameServer]))))

  return gameServers.length < 100 ? IndexStatus.COMPLETED : IndexStatus.IN_PROGRESS;
}

function processResults<T, U>(results: FuseResult<T>[], convert: (item: T) => U, sortBy: (item: U) => number) {
  return results
    .map(result => ({ score: result.score, item: convert(result.item) }))
    .filter(result => result.score < 0.7)
    .sort((a, b) => a.score - b.score || sortBy(b.item) - sortBy(a.item))
    .map(result => result.item);
}

export function searchPlayers(query: string) {
  return processResults(
    fusePlayers.search(query, { limit: 30 }),
    player => indexedPlayers[player.name],
    player => player.playTime
  );
}

export function searchClans(query: string) {
  return processResults(
    fuseClans.search(query, { limit: 30 }),
    clan => indexedClans[clan.name],
    clan => clan.playTime
  );
}

export function searchGameServers(query: string) {
  return processResults(
    fuseGameServers.search(query, { limit: 30 }),
    gameServer => indexedGameServers[gameServer.name],
    gameServer => gameServer.clientCount
  );
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
    console.log(`Restoring from ${DUMP_PATH}`);
    const fileSize = statSync(DUMP_PATH).size;
    console.log(`File size: ${fileSize}`);

    const data = dumpSchema.parse(JSON.parse(readFileSync(DUMP_PATH, 'utf8')));

    if (data.version !== DUMP_VERSION) {
      console.log(`Invalid dump version ${data.version}, expected ${DUMP_VERSION}`);
      return;
    }

    console.log(`Restoring ${Object.keys(data.players).length} players, ${Object.keys(data.clans).length} clans, ${Object.keys(data.gameServers).length} game servers`);

    Object.assign(indexedPlayers, data.players);
    Object.assign(indexedClans, data.clans);
    Object.assign(indexedGameServers, data.gameServers);

    fusePlayers.setCollection(Object.values(indexedPlayers));
    fuseClans.setCollection(Object.values(indexedClans));
    fuseGameServers.setCollection(Object.values(indexedGameServers));

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

async function runInBackground(callback: () => Promise<IndexStatus>) {
  for (; ;) {
    const status = await callback().catch(error => {
      console.error(error);
      captureException(error);
      return IndexStatus.IN_PROGRESS;
    });

    if (status === IndexStatus.COMPLETED) {
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

  runInBackground(updatePlayers);
  runInBackground(updateClans);
  runInBackground(updateGameServers);

  setInterval(dump, minutesToMilliseconds(10));
}
