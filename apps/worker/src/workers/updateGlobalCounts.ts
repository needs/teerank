import { Worker } from "bullmq";
import { prisma } from "../prisma";
import { QUEUE_NAME_UPDATE_GLOBAL_COUNTS } from "@teerank/teerank";
import { bullmqConnection } from "@teerank/teerank";
import { RedisClient, redisClientPromise } from "../redis";
import { minutesToSeconds } from "date-fns";

const GLOBAL_COUNTS_PLAYERS_LAST_UPDATED_AT_KEY = 'global-counts-players-last-updated-at';
const GLOBAL_COUNTS_CLANS_LAST_UPDATED_AT_KEY = 'global-counts-clans-last-updated-at';
const GLOBAL_COUNTS_MAPS_LAST_UPDATED_AT_KEY = 'global-counts-maps-last-updated-at';
const GLOBAL_COUNTS_GAME_TYPES_LAST_UPDATED_AT_KEY = 'global-counts-game-types-last-updated-at';
const GLOBAL_COUNTS_GAME_SERVERS_LAST_UPDATED_AT_KEY = 'global-counts-game-servers-last-updated-at';

const GLOBAL_COUNTS_PLAYERS_KEY = 'global-counts-players';
const GLOBAL_COUNTS_CLANS_KEY = 'global-counts-clans';
const GLOBAL_COUNTS_MAPS_KEY = 'global-counts-maps';
const GLOBAL_COUNTS_GAME_TYPES_KEY = 'global-counts-game-types';
const GLOBAL_COUNTS_GAME_SERVERS_KEY = 'global-counts-game-servers';

export async function updateCount({
  redisClient,
  getNewEntities,
  lastUpdatedAtKey,
  countKey,
}: {
  redisClient: RedisClient;
  getNewEntities: (lastUpdateDate: Date) => Promise<{ createdAt: Date }[]>;
  lastUpdatedAtKey: string;
  countKey: string;
}) {
  const lastUpdateDate = new Date(Number(await redisClient.get(lastUpdatedAtKey) || "0"));

  console.log(`Last update date: ${lastUpdateDate}`);

  const entities = await getNewEntities(lastUpdateDate);

  const newCount = entities.length;
  const newLastUpdatedAt = Math.max(lastUpdateDate.getTime(), entities[entities.length - 1].createdAt.getTime());

  console.log(`New count: ${newCount}`);
  console.log(`New last updated at: ${newLastUpdatedAt}`);

  const pipeline = redisClient.multi();
  pipeline.incrBy(countKey, newCount);
  pipeline.set(lastUpdatedAtKey, newLastUpdatedAt);
  await pipeline.exec();

  console.log(`Global counts saved`);
}

async function updatePlayersCount(redisClient: RedisClient) {
  const getNewEntities = async (lastUpdateDate: Date) => {
    return await prisma.player.findMany({
      select: {
        createdAt: true,
      },
      where: {
        createdAt: {
          gt: lastUpdateDate,
        },
      },
    });
  }

  console.log('Updating players count');
  await updateCount({
    redisClient,
    getNewEntities,
    lastUpdatedAtKey: GLOBAL_COUNTS_PLAYERS_LAST_UPDATED_AT_KEY,
    countKey: GLOBAL_COUNTS_PLAYERS_KEY
  });
}

async function updateClansCount(redisClient: RedisClient) {
  const getNewEntities = async (lastUpdateDate: Date) => {
    return await prisma.clan.findMany({
      select: {
        createdAt: true,
      },
      where: {
        createdAt: {
          gt: lastUpdateDate,
        },
      },
    });
  }

  console.log('Updating clans count');
  await updateCount({
    redisClient,
    getNewEntities,
    lastUpdatedAtKey: GLOBAL_COUNTS_CLANS_LAST_UPDATED_AT_KEY,
    countKey: GLOBAL_COUNTS_CLANS_KEY
  });
}

async function updateGameServersCount(redisClient: RedisClient) {
  const getNewEntities = async (lastUpdateDate: Date) => {
    return await prisma.gameServer.findMany({
      select: {
        createdAt: true,
      },
      where: {
        createdAt: {
          gt: lastUpdateDate,
        },
      },
    });
  }

  console.log('Updating game servers count');
  await updateCount({
    redisClient,
    getNewEntities,
    lastUpdatedAtKey: GLOBAL_COUNTS_GAME_SERVERS_LAST_UPDATED_AT_KEY,
    countKey: GLOBAL_COUNTS_GAME_SERVERS_KEY
  });
}

async function updateMapsCount(redisClient: RedisClient) {
  const getNewEntities = async (lastUpdateDate: Date) => {
    return await prisma.map.findMany({
      select: {
        createdAt: true,
      },
      where: {
        createdAt: {
          gt: lastUpdateDate,
        },
      },
    });
  }

  console.log('Updating maps count');
  await updateCount({
    redisClient,
    getNewEntities,
    lastUpdatedAtKey: GLOBAL_COUNTS_MAPS_LAST_UPDATED_AT_KEY,
    countKey: GLOBAL_COUNTS_MAPS_KEY
  });
}

async function updateGameTypesCount(redisClient: RedisClient) {
  const getNewEntities = async (lastUpdateDate: Date) => {
    return await prisma.gameType.findMany({
      select: {
        createdAt: true,
      },
      where: {
        createdAt: {
          gt: lastUpdateDate,
        },
      },
    });
  }

  console.log('Updating game types count');
  await updateCount({
    redisClient,
    getNewEntities,
    lastUpdatedAtKey: GLOBAL_COUNTS_GAME_TYPES_LAST_UPDATED_AT_KEY,
    countKey: GLOBAL_COUNTS_GAME_TYPES_KEY
  });
}

async function processor() {
  const redisClient = await redisClientPromise;

  await updatePlayersCount(redisClient);
  await updateClansCount(redisClient);
  await updateGameServersCount(redisClient);
  await updateMapsCount(redisClient);
  await updateGameTypesCount(redisClient);
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
