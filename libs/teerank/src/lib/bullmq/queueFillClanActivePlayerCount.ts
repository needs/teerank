import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection, lastCompletedJobDate } from "./config";
import { z } from "zod";
import { minutesToSeconds } from "date-fns";

let fillClanActivePlayerCountQueue: Queue | null = null;

export const QUEUE_NAME_FILL_CLAN_ACTIVE_PLAYER_COUNT = 'fill-clan-active-player-count';

function getQueueFillClanActivePlayerCount() {
  fillClanActivePlayerCountQueue ??= new Queue(QUEUE_NAME_FILL_CLAN_ACTIVE_PLAYER_COUNT, { connection: bullmqConnection });
  return fillClanActivePlayerCountQueue;
}

const schema = z.object({});

export type FillClanActivePlayerCountJobData = z.infer<typeof schema>;

export async function scheduleFillClanActivePlayerCount() {
  const queue = getQueueFillClanActivePlayerCount();
  await queue.add('fill-clan-active-player-count', {}, {
    deduplication: {
      id: 'fill-clan-active-player-count',
    }
  });
}

export async function processFillClanActivePlayerCountJobs(processor: (data: FillClanActivePlayerCountJobData) => Promise<void>) {
  const jobProcessor = async (job: Job) => {
    const data = schema.parse(job.data);
    await processor(data);
  }

  return new Worker(QUEUE_NAME_FILL_CLAN_ACTIVE_PLAYER_COUNT, jobProcessor, {
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

export async function getLastFillClanActivePlayerCountDate() {
  return lastCompletedJobDate(getQueueFillClanActivePlayerCount());
}

export { getQueueFillClanActivePlayerCount };
