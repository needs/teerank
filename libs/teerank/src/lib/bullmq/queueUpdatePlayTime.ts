import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection, lastCompletedJobDate } from "./config";
import { z } from "zod";
import { minutesToSeconds } from "date-fns";
import { getEnvInt } from "../utils";

let updatePlayTimeQueue: Queue | null = null;

const QUEUE_NAME_UPDATE_PLAY_TIME = 'update-play-time';
const UPDATE_PLAY_TIME_CONCURRENCY = getEnvInt('UPDATE_PLAY_TIME_CONCURRENCY', 20);

function getQueueUpdatePlayTime() {
  updatePlayTimeQueue ??= new Queue(QUEUE_NAME_UPDATE_PLAY_TIME, { connection: bullmqConnection });
  return updatePlayTimeQueue;
}

const schema = z.object({
  snapshotId: z.number(),
});

export type UpdatePlayTimeJobData = z.infer<typeof schema>;

export async function scheduleUpdatePlayTime(data: UpdatePlayTimeJobData) {
  const queue = getQueueUpdatePlayTime();
  await queue.add(`snapshot-${data.snapshotId}`, data, {
    removeOnComplete: 1000,
    removeOnFail: 1000,
  });
}

export async function processUpdatePlayTimeJobs(processor: (data: UpdatePlayTimeJobData) => Promise<void>) {
  const jobProcessor = async (job: Job) => {
    const data = schema.parse(job.data);
    await processor(data);
  }

  return new Worker(QUEUE_NAME_UPDATE_PLAY_TIME, jobProcessor, {
    connection: bullmqConnection,
    concurrency: UPDATE_PLAY_TIME_CONCURRENCY,
    removeOnComplete: {
      age: minutesToSeconds(10),
    },
    removeOnFail: {
      count: 1000,
    }
  });
}

export async function cleanUpdatePlayTimeQueue() {
  await getQueueUpdatePlayTime().obliterate({
    force: true,
  });
}

export async function getLastUpdatePlayTimeDate() {
  return lastCompletedJobDate(getQueueUpdatePlayTime());
}

export async function getUpdatePlayTimeWaitingCount() {
  const queue = getQueueUpdatePlayTime();
  return queue.getWaitingCount();
}
