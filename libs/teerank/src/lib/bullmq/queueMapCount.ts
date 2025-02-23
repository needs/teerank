import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection, lastCompletedJobDate } from "./config";
import { z } from "zod";
import { minutesToSeconds } from "date-fns";
import { getEnvInt } from "../utils";

let mapCountQueue: Queue | null = null;

const QUEUE_NAME_MAP_COUNT = 'map-count';
const UPDATE_MAPS_COUNTS_CONCURRENCY = getEnvInt('UPDATE_MAPS_COUNTS_CONCURRENCY', 5);

function getQueueMapCount() {
  mapCountQueue ??= new Queue(QUEUE_NAME_MAP_COUNT, { connection: bullmqConnection });
  return mapCountQueue;
}

const schema = z.object({
  gameTypeName: z.string(),
  mapName: z.string(),
  mapId: z.number(),
});

export type MapCountJobData = z.infer<typeof schema>;

export async function scheduleMapCount(data: MapCountJobData) {
  const queue = getQueueMapCount();
  await queue.add(`${data.gameTypeName} - ${data.mapName}`, data, {
    deduplication: {
      id: data.mapId.toString(),
    }
  });
}

export async function processMapCountJobs(processor: (data: MapCountJobData) => Promise<void>) {
  const jobProcessor = async (job: Job) => {
    const data = schema.parse(job.data);
    await processor(data);
  }

  return new Worker(QUEUE_NAME_MAP_COUNT, jobProcessor, {
    connection: bullmqConnection,
    concurrency: UPDATE_MAPS_COUNTS_CONCURRENCY,
    removeOnComplete: {
      age: minutesToSeconds(10),
    },
    removeOnFail: {
      count: 1000,
    }
  });
}

export async function cleanMapCountQueue() {
  await getQueueMapCount().obliterate({
    force: true,
  });
}

export async function getLastMapCountDate() {
  return lastCompletedJobDate(getQueueMapCount());
}
