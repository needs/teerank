import { Worker } from "bullmq";
import { prisma } from "../prisma";
import {
  QUEUE_NAME_UPDATE_GLOBAL_COUNTS,
  getGlobalCountsLastUpdatedAt,
  incrementGlobalPlayerCount,
  incrementGlobalClanCount,
  incrementGlobalGameServerCount,
  incrementGlobalMapCount,
  incrementGlobalGameTypeCount
} from "@teerank/teerank";
import { bullmqConnection } from "@teerank/teerank";
import { redis } from "../redis";
import { minutesToSeconds } from "date-fns";

function getNewLastUpdatedAt(entities: { createdAt: Date }[]) {
  return new Date(Math.max(...entities.map(entity => entity.createdAt.getTime())));
}

function getNewLastId(entities: { id: number }[]) {
  return Math.max(...entities.map(entity => entity.id));
}

export async function updatePlayersCount(lastUpdateDate: Date) {
  const players = await prisma.player.findMany({
    select: {
      createdAt: true,
    },
    where: {
      createdAt: {
        gt: lastUpdateDate,
      },
    },
    orderBy: {
      createdAt: 'asc',
    },
    take: 1000,
  });

  console.log(`Found ${players.length} players to update (${lastUpdateDate.toISOString()} - ${new Date().toISOString()})`);

  if (players.length > 0) {
    const newLastUpdatedAt = getNewLastUpdatedAt(players);
    await incrementGlobalPlayerCount(redis, players.length, newLastUpdatedAt);
  }
}

export async function updateClansCount(lastUpdateDate: Date) {
  const clans = await prisma.clan.findMany({
    select: {
      createdAt: true,
    },
    where: {
      createdAt: {
        gt: lastUpdateDate,
      },
    },
    orderBy: {
      createdAt: 'asc',
    },
    take: 1000,
  });

  if (clans.length > 0) {
    const newLastUpdatedAt = getNewLastUpdatedAt(clans);
    await incrementGlobalClanCount(redis, clans.length, newLastUpdatedAt);
  }
}

export async function updateGameTypesCount(lastUpdateDate: Date) {
  const gameTypes = await prisma.gameType.findMany({
    select: {
      createdAt: true,
    },
    where: {
      createdAt: {
        gt: lastUpdateDate,
      },
    },
    orderBy: {
      createdAt: 'asc',
    },
    take: 1000,
  });

  if (gameTypes.length > 0) {
    const newLastUpdatedAt = getNewLastUpdatedAt(gameTypes);
    await incrementGlobalGameTypeCount(redis, gameTypes.length, newLastUpdatedAt);
  }
}

export async function updateGameServersCount(lastUpdateId: number) {
  const gameServers = await prisma.gameServer.findMany({
    select: {
      id: true,
    },
    where: {
      id: {
        gt: lastUpdateId,
      },
    },
    orderBy: {
      id: 'asc',
    },
    take: 1000,
  });

  if (gameServers.length > 0) {
    const newLastUpdatedId = getNewLastId(gameServers);
    await incrementGlobalGameServerCount(redis, gameServers.length, newLastUpdatedId);
  }
}

export async function updateMapsCount(lastUpdateId: number) {
  const maps = await prisma.map.findMany({
    select: {
      id: true,
    },
    where: {
      id: {
        gt: lastUpdateId,
      },
    },
    orderBy: {
      id: 'asc',
    },
    take: 1000,
  });

  if (maps.length > 0) {
    const newLastUpdatedId = getNewLastId(maps);
    await incrementGlobalMapCount(redis, maps.length, newLastUpdatedId);
  }
}

async function processor() {
  const {
    playersLastUpdatedAt,
    clansLastUpdatedAt,
    gameTypesLastUpdatedAt,
    mapsLastUpdatedId,
    gameServersLastUpdatedId
  } = await getGlobalCountsLastUpdatedAt(redis);

  await updatePlayersCount(playersLastUpdatedAt);
  await updateClansCount(clansLastUpdatedAt);
  await updateGameTypesCount(gameTypesLastUpdatedAt);
  await updateGameServersCount(gameServersLastUpdatedId);
  await updateMapsCount(mapsLastUpdatedId);
}

export async function startUpdateGlobalCountsWorker() {
  return new Worker(QUEUE_NAME_UPDATE_GLOBAL_COUNTS, processor, {
    connection: bullmqConnection,
    concurrency: 1,
    removeOnComplete: {
      age: minutesToSeconds(10),
    },
    removeOnFail: {
      count: 1000,
    }
  });
}
