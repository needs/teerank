import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection } from "./config";
import { hoursToSeconds } from "date-fns";

let ddnetOnlineQueue: Queue | null = null;

const QUEUE_NAME_DDNET_ONLINE = 'ddnet-online';

function getQueueDdnetOnline() {
  ddnetOnlineQueue ??= new Queue(QUEUE_NAME_DDNET_ONLINE, { connection: bullmqConnection });
  return ddnetOnlineQueue;
}

export async function scheduleDdnetOnline() {
  const queue = getQueueDdnetOnline();
  await queue.add('ddnet-online', {}, {
    deduplication: {
      id: 'ddnet-online',
    }
  });
}

export async function processDdnetOnlineJobs(processor: () => Promise<void>) {
  const jobProcessor = async (_job: Job) => {
    await processor();
  }

  return new Worker(QUEUE_NAME_DDNET_ONLINE, jobProcessor, {
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

export async function cleanDdnetOnlineQueue() {
  await getQueueDdnetOnline().obliterate({
    force: true,
  });
}

export async function getDdnetOnlineFailedCount() {
  return getQueueDdnetOnline().getFailedCount();
}
