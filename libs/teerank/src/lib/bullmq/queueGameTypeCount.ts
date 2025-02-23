import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection, lastCompletedJobDate } from "./config";
import { z } from "zod";
import { minutesToSeconds } from "date-fns";
import { getEnvInt } from "../utils";

let gameTypeCountQueue: Queue | null = null;

const QUEUE_NAME_GAME_TYPE_COUNT = 'game-type-count';
const UPDATE_GAME_TYPES_COUNTS_CONCURRENCY = getEnvInt('UPDATE_GAME_TYPES_COUNTS_CONCURRENCY', 5);

function getQueueGameTypeCount() {
  gameTypeCountQueue ??= new Queue(QUEUE_NAME_GAME_TYPE_COUNT, { connection: bullmqConnection });
  return gameTypeCountQueue;
}

const schema = z.object({
  gameTypeName: z.string(),
});

export type GameTypeCountJobData = z.infer<typeof schema>;

export async function scheduleGameTypeCount(data: GameTypeCountJobData) {
  const queue = getQueueGameTypeCount();
  await queue.add(data.gameTypeName, data, {
    deduplication: {
      id: data.gameTypeName,
    }
  });
}

export async function processGameTypeCountJobs(processor: (data: GameTypeCountJobData) => Promise<void>) {
  const jobProcessor = async (job: Job) => {
    const data = schema.parse(job.data);
    await processor(data);
  }

  return new Worker(QUEUE_NAME_GAME_TYPE_COUNT, jobProcessor, {
    connection: bullmqConnection,
    concurrency: UPDATE_GAME_TYPES_COUNTS_CONCURRENCY,
    removeOnComplete: {
      age: minutesToSeconds(10),
    },
    removeOnFail: {
      count: 1000,
    }
  });
}

export async function cleanGameTypeCountQueue() {
  await getQueueGameTypeCount().obliterate({
    force: true,
  });
}

export async function getLastGameTypeCountDate() {
  return lastCompletedJobDate(getQueueGameTypeCount());
}
