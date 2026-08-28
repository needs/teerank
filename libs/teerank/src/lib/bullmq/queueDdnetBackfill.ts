import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection } from "./config";
import { hoursToSeconds } from "date-fns";

let ddnetBackfillQueue: Queue | null = null;

const QUEUE_NAME_DDNET_BACKFILL = 'ddnet-backfill';

function getQueueDdnetBackfill() {
  ddnetBackfillQueue ??= new Queue(QUEUE_NAME_DDNET_BACKFILL, { connection: bullmqConnection });
  return ddnetBackfillQueue;
}

export async function scheduleDdnetBackfill() {
  const queue = getQueueDdnetBackfill();
  await queue.add('ddnet-backfill-tick', {}, {
    deduplication: {
      id: 'ddnet-backfill-tick',
    }
  });
}

export async function processDdnetBackfillJobs(processor: () => Promise<void>) {
  const jobProcessor = async (_job: Job) => {
    await processor();
  }

  return new Worker(QUEUE_NAME_DDNET_BACKFILL, jobProcessor, {
    connection: bullmqConnection,
    concurrency: 1,
    removeOnComplete: {
      age: hoursToSeconds(6),
    },
    removeOnFail: {
      count: 1000,
    }
  });
}

export async function cleanDdnetBackfillQueue() {
  await getQueueDdnetBackfill().obliterate({
    force: true,
  });
}

export async function getDdnetBackfillFailedCount() {
  return getQueueDdnetBackfill().getFailedCount();
}
