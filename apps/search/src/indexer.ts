import { PrismaClient } from "@prisma/client";
import { max } from "lodash";
import Fuse, { FuseResult } from "fuse.js";
import { IndexedPlayer, IndexedClan, randomRange, wait } from "@teerank/teerank";
import { minutesToMilliseconds } from "date-fns";

const prisma = new PrismaClient();

const indexedPlayers: Record<string, IndexedPlayer> = {};
const fusePlayers = new Fuse(Object.values(indexedPlayers), { keys: ['name'], includeScore: true });

const indexedClans: Record<string, IndexedClan> = {};
const fuseClans = new Fuse(Object.values(indexedClans), { keys: ['name'], includeScore: true });

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

  Object.assign(indexedClans, Object.fromEntries(clans.map(clan => ([clan.name, { ...clan, playTime: Number(clan.playTime) }]))))
  fuseClans.setCollection(Object.values(indexedClans));

  return clans.length < 100 ? IndexStatus.COMPLETED : IndexStatus.IN_PROGRESS;
}

async function indexClans() {
  fuseClans.setCollection(Object.values(indexedClans));
  console.log(`Indexed ${Object.keys(indexedClans).length} clans`);
}

function processResults(results: FuseResult<IndexedPlayer | IndexedClan>[]) {
  return results.filter(result => result.score < 0.3).sort((a, b) => a.score - b.score || b.item.playTime - a.item.playTime).map(result => result.item);
}

export function searchPlayers(query: string) {
  return processResults(fusePlayers.search(query, { limit: 30 }));
}

export function searchClans(query: string) {
  return processResults(fuseClans.search(query, { limit: 30 }));
}

async function runInBackground(update: () => Promise<IndexStatus>, index: () => Promise<void>) {
  for (; ;) {
    const status = await update();
    if (status === IndexStatus.COMPLETED) {
      await index();
      const spread = randomRange(minutesToMilliseconds(4), minutesToMilliseconds(6));
      await wait(spread);
    }
  }
}

export function startBackgroundIndexing() {
  runInBackground(updatePlayers, indexPlayers);
  runInBackground(updateClans, indexClans);
}
