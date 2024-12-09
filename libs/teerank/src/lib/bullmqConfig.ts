import { Queue, RedisOptions } from "bullmq";
import { REDIS_HOST, REDIS_PORT } from "./redisConfig";

export const bullmqConnection: RedisOptions = {
  host: REDIS_HOST,
  port: REDIS_PORT,
  family: 6,
};

export const QUEUE_NAME_POLL_MASTER_SERVER = 'poll-master-server';
export const QUEUE_NAME_POLL_GAME_SERVER = 'poll-game-server';
export const QUEUE_NAME_UPDATE_PLAY_TIME = 'update-play-time';
export const QUEUE_NAME_RANK_PLAYER = 'rank-player';
export const QUEUE_NAME_GAME_TYPE_COUNT = 'game-type-count';
export const QUEUE_NAME_MAP_COUNT = 'map-count';

export function getQueuePollMasterServer() {
  return new Queue(QUEUE_NAME_POLL_MASTER_SERVER, { connection: bullmqConnection });
}

export function getQueuePollGameServer() {
  return new Queue(QUEUE_NAME_POLL_GAME_SERVER, { connection: bullmqConnection });
}

export function getQueueUpdatePlayTime() {
  return new Queue(QUEUE_NAME_UPDATE_PLAY_TIME, { connection: bullmqConnection });
}

export function getQueueRankPlayer() {
  return new Queue(QUEUE_NAME_RANK_PLAYER, { connection: bullmqConnection });
}

export function getQueueGameTypeCount() {
  return new Queue(QUEUE_NAME_GAME_TYPE_COUNT, { connection: bullmqConnection });
}

export function getQueueMapCount() {
  return new Queue(QUEUE_NAME_MAP_COUNT, { connection: bullmqConnection });
}

export async function cleanQueue(queue: Queue) {
  console.log('Cleaning queue', queue.name);
  await queue.obliterate({
    force: true,
  });
  console.log('Queue cleaned', queue.name);
}

export async function lastCompletedJobDate(queue: Queue) {
  const lastCompletedJob = await queue.getCompleted();
  return lastCompletedJob.length > 0 ? lastCompletedJob[0].processedOn : null;
}
