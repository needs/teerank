import { Queue, RedisOptions } from "bullmq";
import { REDIS_HOST, REDIS_PORT, REDIS_FAMILY } from "../redisConfig";

export const bullmqConnection: RedisOptions = {
  host: REDIS_HOST,
  port: REDIS_PORT,
  family: REDIS_FAMILY,
};

export async function lastCompletedJobDate(queue: Queue) {
  const lastCompletedJob = await queue.getCompleted();
  return lastCompletedJob.length > 0 ? lastCompletedJob[0].processedOn : null;
}
