import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection, lastCompletedJobDate } from "./config";
import { z } from "zod";
import { minutesToSeconds } from "date-fns";

let updateGlobalCountsQueue: Queue | null = null;

const QUEUE_NAME_UPDATE_GLOBAL_COUNTS = 'update-global-counts';

function getQueueUpdateGlobalCounts() {
  updateGlobalCountsQueue ??= new Queue(QUEUE_NAME_UPDATE_GLOBAL_COUNTS, { connection: bullmqConnection });
  return updateGlobalCountsQueue;
}

const schema = z.object({});

export type UpdateGlobalCountsJobData = z.infer<typeof schema>;

export async function scheduleUpdateGlobalCounts() {
  const queue = getQueueUpdateGlobalCounts();
  await queue.add(QUEUE_NAME_UPDATE_GLOBAL_COUNTS, {}, {
    deduplication: {
      id: 'update-global-counts',
    }
  });
}

export async function processUpdateGlobalCountsJobs(processor: (data: UpdateGlobalCountsJobData) => Promise<void>) {
  const jobProcessor = async (job: Job) => {
    const data = schema.parse(job.data);
    await processor(data);
  }

  return new Worker(QUEUE_NAME_UPDATE_GLOBAL_COUNTS, jobProcessor, {
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

export async function cleanUpdateGlobalCountsQueue() {
  await getQueueUpdateGlobalCounts().obliterate({
    force: true,
  });
}

export async function getLastUpdateGlobalCountsDate() {
  return lastCompletedJobDate(getQueueUpdateGlobalCounts());
}
