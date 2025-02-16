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
import { Redis } from "ioredis";
import { redis } from "../redis";
import { minutesToSeconds } from "date-fns";

function getNewLastUpdatedAt(entities: { createdAt: Date }[]) {
  return new Date(Math.max(...entities.map(entity => entity.createdAt.getTime())));
}

export async function updatePlayersCount(redis: Redis, lastUpdateDate: Date) {
  const players = await prisma.player.findMany({
    select: {
      createdAt: true,
    },
    where: {
      createdAt: {
        gt: lastUpdateDate,
      },
    },
  });

  const newLastUpdatedAt = getNewLastUpdatedAt(players);

  await incrementGlobalPlayerCount(redis, players.length, newLastUpdatedAt);
}

export async function updateClansCount(redis: Redis, lastUpdateDate: Date) {
  const clans = await prisma.clan.findMany({
    select: {
      createdAt: true,
    },
    where: {
      createdAt: {
        gt: lastUpdateDate,
      },
    },
  });

  const newLastUpdatedAt = getNewLastUpdatedAt(clans);

  await incrementGlobalClanCount(redis, clans.length, newLastUpdatedAt);
}

export async function updateGameServersCount(redis: Redis, lastUpdateDate: Date) {
  const gameServers = await prisma.gameServer.findMany({
    select: {
      createdAt: true,
    },
    where: {
      createdAt: {
        gt: lastUpdateDate,
      },
    },
  });

  const newLastUpdatedAt = getNewLastUpdatedAt(gameServers);

  await incrementGlobalGameServerCount(redis, gameServers.length, newLastUpdatedAt);
}

export async function updateMapsCount(redis: Redis, lastUpdateDate: Date) {
  const maps = await prisma.map.findMany({
    select: {
      createdAt: true,
    },
    where: {
      createdAt: {
        gt: lastUpdateDate,
      },
    },
  });

  const newLastUpdatedAt = getNewLastUpdatedAt(maps);

  await incrementGlobalMapCount(redis, maps.length, newLastUpdatedAt);
}

export async function updateGameTypesCount(redis: Redis, lastUpdateDate: Date) {
  const gameTypes = await prisma.gameType.findMany({
    select: {
      createdAt: true,
    },
    where: {
      createdAt: {
        gt: lastUpdateDate,
      },
    },
  });

  const newLastUpdatedAt = getNewLastUpdatedAt(gameTypes);

  await incrementGlobalGameTypeCount(redis, gameTypes.length, newLastUpdatedAt);
}

async function processor() {
  const {
    playersLastUpdatedAt,
    clansLastUpdatedAt,
    mapsLastUpdatedAt,
    gameTypesLastUpdatedAt,
    gameServersLastUpdatedAt
  } = await getGlobalCountsLastUpdatedAt(redis);

  await updatePlayersCount(redis, playersLastUpdatedAt);
  await updateClansCount(redis, clansLastUpdatedAt);
  await updateGameServersCount(redis, gameServersLastUpdatedAt);
  await updateMapsCount(redis, mapsLastUpdatedAt);
  await updateGameTypesCount(redis, gameTypesLastUpdatedAt);
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
