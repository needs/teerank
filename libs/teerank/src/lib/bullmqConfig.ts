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

export function getQueuePollMasterServer() {
  return new Queue(QUEUE_NAME_POLL_MASTER_SERVER, { connection: bullmqConnection });
}

export function getQueuePollGameServer() {
  return new Queue(QUEUE_NAME_POLL_GAME_SERVER, { connection: bullmqConnection });
}

export function getQueueUpdatePlayTime() {
  return new Queue(QUEUE_NAME_UPDATE_PLAY_TIME, { connection: bullmqConnection });
}

export async function removeAllSchedulers(queue: Queue) {
  const schedulers = await queue.getJobSchedulers();

  await Promise.all(
    schedulers.map((scheduler) => {
      if (scheduler.id) {
        queue.removeJobScheduler(scheduler.id);
      }
    }),
  );
}

export async function lastCompletedJobDate(queue: Queue) {
  const lastCompletedJob = await queue.getCompleted();
  return lastCompletedJob.length > 0 ? lastCompletedJob[0].processedOn : null;
}
