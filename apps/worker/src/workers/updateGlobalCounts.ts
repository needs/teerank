import { Worker } from "bullmq";
import { prisma } from "../prisma";
import {
  QUEUE_NAME_UPDATE_GLOBAL_COUNTS,
  getGlobalCountsLastId,
  incrementGlobalPlayerCount,
  incrementGlobalClanCount,
  incrementGlobalGameServerCount,
  incrementGlobalMapCount,
  incrementGlobalGameTypeCount
} from "@teerank/teerank";
import { bullmqConnection } from "@teerank/teerank";
import { redis } from "../redis";
import { minutesToSeconds } from "date-fns";

function getNewLastId(entities: { id: number }[]) {
  return Math.max(...entities.map(entity => entity.id));
}

export async function updatePlayersCount(lastId: number) {
  const players = await prisma.player.findMany({
    select: {
      id: true,
    },
    where: {
      id: {
        gt: lastId,
      },
    },
    orderBy: {
      id: 'asc',
    },
    take: 1000,
  });

  if (players.length > 0) {
    const newLastUpdatedId = getNewLastId(players);
    await incrementGlobalPlayerCount(redis, players.length, newLastUpdatedId);
  }
}

export async function updateClansCount(lastId: number) {
  const clans = await prisma.clan.findMany({
    select: {
      id: true,
    },
    where: {
      id: {
        gt: lastId,
      },
    },
    orderBy: {
      id: 'asc',
    },
    take: 1000,
  });

  if (clans.length > 0) {
    const newLastUpdatedId = getNewLastId(clans);
    await incrementGlobalClanCount(redis, clans.length, newLastUpdatedId);
  }
}

export async function updateGameTypesCount(lastId: number) {
  const gameTypes = await prisma.gameType.findMany({
    select: {
      id: true,
    },
    where: {
      id: {
        gt: lastId,
      },
    },
    orderBy: {
      id: 'asc',
    },
    take: 1000,
  });

  if (gameTypes.length > 0) {
    const newLastUpdatedId = getNewLastId(gameTypes);
    await incrementGlobalGameTypeCount(redis, gameTypes.length, newLastUpdatedId);
  }
}

export async function updateGameServersCount(lastId: number) {
  const gameServers = await prisma.gameServer.findMany({
    select: {
      id: true,
    },
    where: {
      id: {
        gt: lastId,
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

export async function updateMapsCount(lastId: number) {
  const maps = await prisma.map.findMany({
    select: {
      id: true,
    },
    where: {
      id: {
        gt: lastId,
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
    playersLastId,
    clansLastId,
    gameTypesLastId,
    mapsLastId,
    gameServersLastId
  } = await getGlobalCountsLastId(redis);

  await updatePlayersCount(playersLastId);
  await updateClansCount(clansLastId);
  await updateGameTypesCount(gameTypesLastId);
  await updateGameServersCount(gameServersLastId);
  await updateMapsCount(mapsLastId);
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
