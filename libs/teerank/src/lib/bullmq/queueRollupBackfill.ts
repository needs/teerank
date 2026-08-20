import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection } from "./config";
import { z } from "zod";
import { hoursToSeconds } from "date-fns";
import { utcDaySchema } from "../schemas";

let rollupBackfillQueue: Queue | null = null;

const QUEUE_NAME_ROLLUP_BACKFILL = 'rollup-backfill';

function getQueueRollupBackfill() {
  rollupBackfillQueue ??= new Queue(QUEUE_NAME_ROLLUP_BACKFILL, { connection: bullmqConnection });
  return rollupBackfillQueue;
}

const schema = z.object({
  day: utcDaySchema,
});

export type RollupBackfillJobData = z.infer<typeof schema>;

export async function scheduleRollupBackfill(data: RollupBackfillJobData) {
  const queue = getQueueRollupBackfill();
  await queue.add(`rollup-backfill-${data.day}`, data, {
    deduplication: {
      id: `rollup-backfill-${data.day}`,
    }
  });
}

export async function processRollupBackfillJobs(processor: (data: RollupBackfillJobData) => Promise<void>) {
  const jobProcessor = async (job: Job) => {
    const data = schema.parse(job.data);
    await processor(data);
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
