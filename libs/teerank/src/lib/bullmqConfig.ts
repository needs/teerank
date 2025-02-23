import { Queue, RedisOptions } from "bullmq";
import { REDIS_HOST, REDIS_PORT, REDIS_FAMILY } from "./redisConfig";
import { cleanPollMasterServerQueue } from "./bullmq/queuePollMasterServer";
import { cleanPollGameServerQueue } from "./bullmq/queuePollGameServer";

export const bullmqConnection: RedisOptions = {
  host: REDIS_HOST,
  port: REDIS_PORT,
  family: REDIS_FAMILY,
};

export const QUEUE_NAME_UPDATE_PLAY_TIME = 'update-play-time';
export const QUEUE_NAME_RANK_PLAYER = 'rank-player';
export const QUEUE_NAME_GAME_TYPE_COUNT = 'game-type-count';
export const QUEUE_NAME_MAP_COUNT = 'map-count';
export const QUEUE_NAME_FILL_CLAN_ACTIVE_PLAYER_COUNT = 'fill-clan-active-player-count';
export const QUEUE_NAME_UPDATE_GLOBAL_COUNTS = 'update-global-counts';

// Queue instances
let updatePlayTimeQueue: Queue | null = null;
let rankPlayerQueue: Queue | null = null;
let gameTypeCountQueue: Queue | null = null;
let mapCountQueue: Queue | null = null;
let fillClanActivePlayerCountQueue: Queue | null = null;
let updateGlobalCountsQueue: Queue | null = null;

export function getQueueUpdatePlayTime() {
  if (!updatePlayTimeQueue) {
    updatePlayTimeQueue = new Queue(QUEUE_NAME_UPDATE_PLAY_TIME, { connection: bullmqConnection });
  }
  return updatePlayTimeQueue;
}

export function getQueueRankPlayer() {
  if (!rankPlayerQueue) {
    rankPlayerQueue = new Queue(QUEUE_NAME_RANK_PLAYER, { connection: bullmqConnection });
  }
  return rankPlayerQueue;
}

export function getQueueGameTypeCount() {
  if (!gameTypeCountQueue) {
    gameTypeCountQueue = new Queue(QUEUE_NAME_GAME_TYPE_COUNT, { connection: bullmqConnection });
  }
  return gameTypeCountQueue;
}

export function getQueueMapCount() {
  if (!mapCountQueue) {
    mapCountQueue = new Queue(QUEUE_NAME_MAP_COUNT, { connection: bullmqConnection });
  }
  return mapCountQueue;
}

export function getQueueFillClanActivePlayerCount() {
  if (!fillClanActivePlayerCountQueue) {
    fillClanActivePlayerCountQueue = new Queue(QUEUE_NAME_FILL_CLAN_ACTIVE_PLAYER_COUNT, { connection: bullmqConnection });
  }
  return fillClanActivePlayerCountQueue;
}

export function getQueueUpdateGlobalCounts() {
  if (!updateGlobalCountsQueue) {
    updateGlobalCountsQueue = new Queue(QUEUE_NAME_UPDATE_GLOBAL_COUNTS, { connection: bullmqConnection });
  }
  return updateGlobalCountsQueue;
}

export async function cleanQueue(queue: Queue) {
  console.log('Cleaning queue', queue.name);
  await queue.obliterate({
    force: true,
  });
  console.log('Queue cleaned', queue.name);
}

export async function cleanAllQueues() {
  await Promise.all([
    cleanPollMasterServerQueue(),
    cleanPollGameServerQueue(),
    cleanQueue(getQueueGameTypeCount()),
    cleanQueue(getQueueMapCount()),
    cleanQueue(getQueueFillClanActivePlayerCount()),
    cleanQueue(getQueueUpdateGlobalCounts()),
    cleanQueue(getQueueUpdatePlayTime()),
    cleanQueue(getQueueRankPlayer()),
  ]);
}

export async function lastCompletedJobDate(queue: Queue) {
  const lastCompletedJob = await queue.getCompleted();
  return lastCompletedJob.length > 0 ? lastCompletedJob[0].processedOn : null;
}
