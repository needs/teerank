import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection } from "./config";
import { z } from "zod";
import { hoursToSeconds } from "date-fns";
import { utcDaySchema } from "../schemas";

let rollupDayQueue: Queue | null = null;

const QUEUE_NAME_ROLLUP_DAY = 'rollup-day';

function getQueueRollupDay() {
  rollupDayQueue ??= new Queue(QUEUE_NAME_ROLLUP_DAY, { connection: bullmqConnection });
  return rollupDayQueue;
}

const schema = z.object({
  day: utcDaySchema,
});

export type RollupDayJobData = z.infer<typeof schema>;

export async function scheduleRollupDay(data: RollupDayJobData) {
  const queue = getQueueRollupDay();
  await queue.add(`rollup-day-${data.day}`, data, {
    deduplication: {
      id: `rollup-day-${data.day}`,
    }
  });
}

export async function processRollupDayJobs(processor: (data: RollupDayJobData) => Promise<void>) {
  const jobProcessor = async (job: Job) => {
    const data = schema.parse(job.data);
    await processor(data);
  }

  return new Worker(QUEUE_NAME_ROLLUP_DAY, jobProcessor, {
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

export async function cleanRollupDayQueue() {
  await getQueueRollupDay().obliterate({
    force: true,
  });
}
