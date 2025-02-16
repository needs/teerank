import { Worker } from "bullmq";
import { prisma } from "../prisma";
import { QUEUE_NAME_FILL_CLAN_ACTIVE_PLAYER_COUNT } from "@teerank/teerank";
import { bullmqConnection } from "@teerank/teerank";
import { redis } from "../redis";
import { minutesToSeconds } from "date-fns";

const MIN_CREATED_AT_KEY = 'fill-clan-active-player-count-min-created-at';

async function processor() {
  const minCreatedAt = new Date(Number(await redis.get(MIN_CREATED_AT_KEY) || "0"));
  console.log(`Min created at: ${minCreatedAt}`);

  const clans = await prisma.clan.findMany({
    select: {
      name: true,
      createdAt: true,
      _count: {
        select: {
          players: true,
        },
      },
    },

    where: {
      createdAt: {
        gt: minCreatedAt,
      },
    },

    orderBy: {
      createdAt: 'asc',
    },

    take: 100,
  });

  if (clans.length === 0) {
    console.log('No clans to fill');

    return {
      minCreatedAt,
      clansCount: 0,
    };
  }

  console.log(`Filling clan active player count for ${clans.length} clans`);

  for (const clan of clans) {
    await prisma.clan.update({
      where: { name: clan.name },
      data: { activePlayerCount: clan._count.players },
    });
  }

  console.log(`Filled clan active player count for ${clans.length} clans`);

  const newMinCreatedAt = clans[clans.length - 1].createdAt;
  await redis.set(MIN_CREATED_AT_KEY, newMinCreatedAt.getTime());
  console.log(`Updated min created at to ${newMinCreatedAt.toISOString()}`);

  return {
    minCreatedAt: newMinCreatedAt,
    clansCount: clans.length,
  };
}

export async function startFillClanActivePlayerCountWorker() {
  return new Worker(QUEUE_NAME_FILL_CLAN_ACTIVE_PLAYER_COUNT, processor, {
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
