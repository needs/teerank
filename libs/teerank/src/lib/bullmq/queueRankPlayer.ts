import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection, lastCompletedJobDate } from "./config";
import { z } from "zod";
import { minutesToSeconds } from "date-fns";
import { getEnvInt } from "../utils";

let rankPlayerQueue: Queue | null = null;

const QUEUE_NAME_RANK_PLAYER = 'rank-player';
const RANK_PLAYER_CONCURRENCY = getEnvInt('RANK_PLAYER_CONCURRENCY', 20);


function getQueueRankPlayer() {
  rankPlayerQueue ??= new Queue(QUEUE_NAME_RANK_PLAYER, { connection: bullmqConnection });
  return rankPlayerQueue;
}

const schema = z.object({
  snapshotId: z.number(),
});

export type RankPlayerJobData = z.infer<typeof schema>;

export async function scheduleRankPlayer(data: RankPlayerJobData) {
  const queue = getQueueRankPlayer();
  await queue.add(`snapshot-${data.snapshotId}`, data, {
    removeOnComplete: 1000,
    removeOnFail: 1000,
  });
}

export async function processRankPlayerJobs(processor: (data: RankPlayerJobData) => Promise<void>) {
  const jobProcessor = async (job: Job) => {
    const data = schema.parse(job.data);
    await processor(data);
  }

  return new Worker(QUEUE_NAME_RANK_PLAYER, jobProcessor, {
    connection: bullmqConnection,
    concurrency: RANK_PLAYER_CONCURRENCY,
    removeOnComplete: {
      age: minutesToSeconds(10),
    },
    removeOnFail: {
      count: 1000,
    }
  });
}

export async function cleanRankPlayerQueue() {
  await getQueueRankPlayer().obliterate({
    force: true,
  });
}

export async function getLastRankPlayerDate() {
  return lastCompletedJobDate(getQueueRankPlayer());
}

export async function getRankPlayerWaitingCount() {
  const queue = getQueueRankPlayer();
  return queue.getWaitingCount();
}
