import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection } from "./config";
import { hoursToSeconds } from "date-fns";

let rollupBackfillQueue: Queue | null = null;

const QUEUE_NAME_ROLLUP_BACKFILL = 'rollup-backfill';

function getQueueRollupBackfill() {
  rollupBackfillQueue ??= new Queue(QUEUE_NAME_ROLLUP_BACKFILL, { connection: bullmqConnection });
  return rollupBackfillQueue;
}

export async function scheduleRollupBackfill() {
  const queue = getQueueRollupBackfill();
  await queue.add('rollup-backfill-scan', {}, {
    deduplication: {
      id: 'rollup-backfill-scan',
    }
  });
}

export async function processRollupBackfillJobs(processor: () => Promise<void>) {
  const jobProcessor = async (_job: Job) => {
    await processor();
  }

  return new Worker(QUEUE_NAME_ROLLUP_BACKFILL, jobProcessor, {
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

export async function cleanRollupBackfillQueue() {
  await getQueueRollupBackfill().obliterate({
    force: true,
  });
}

export async function getRollupBackfillFailedCount() {
  return getQueueRollupBackfill().getFailedCount();
}
