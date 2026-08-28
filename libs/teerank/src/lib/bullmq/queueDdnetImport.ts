import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection } from "./config";
import { hoursToSeconds } from "date-fns";

let ddnetImportQueue: Queue | null = null;

const QUEUE_NAME_DDNET_IMPORT = 'ddnet-import';

function getQueueDdnetImport() {
  ddnetImportQueue ??= new Queue(QUEUE_NAME_DDNET_IMPORT, { connection: bullmqConnection });
  return ddnetImportQueue;
}

export async function scheduleDdnetImport() {
  const queue = getQueueDdnetImport();
  await queue.add('ddnet-import', {}, {
    deduplication: {
      id: 'ddnet-import',
    }
  });
}

export async function processDdnetImportJobs(processor: () => Promise<void>) {
  const jobProcessor = async (_job: Job) => {
    await processor();
  }

  return new Worker(QUEUE_NAME_DDNET_IMPORT, jobProcessor, {
    connection: bullmqConnection,
    concurrency: 1,
    removeOnComplete: {
      age: hoursToSeconds(48),
    },
    removeOnFail: {
      count: 1000,
    }
  });
}

export async function cleanDdnetImportQueue() {
  await getQueueDdnetImport().obliterate({
    force: true,
  });
}

export async function getDdnetImportFailedCount() {
  return getQueueDdnetImport().getFailedCount();
}
