import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection, lastCompletedJobDate } from "./config";
import { z } from "zod";
import { minutesToSeconds } from "date-fns";

let pollMasterServerQueue: Queue | null = null;

const QUEUE_NAME_POLL_MASTER_SERVER = 'poll-master-server';

function getQueuePollMasterServer() {
  pollMasterServerQueue ??= new Queue(QUEUE_NAME_POLL_MASTER_SERVER, { connection: bullmqConnection });
  return pollMasterServerQueue;
}

const schema = z.object({
  address: z.string(),
  port: z.number(),
});

export type PollMasterServerJobData = z.infer<typeof schema>;

export async function schedulePollMasterServer(data: PollMasterServerJobData) {
  const queue = getQueuePollMasterServer();
  await queue.add(`${data.address}:${data.port}`, data, {
    deduplication: {
      id: `${data.address}:${data.port}`,
    }
  });
}

export async function processPollMasterServerJobs(processor: (data: PollMasterServerJobData) => Promise<void>) {
  const jobProcessor = async (job: Job) => {
    const data = schema.parse(job.data);
    await processor(data);
  }

  return new Worker(QUEUE_NAME_POLL_MASTER_SERVER, jobProcessor, {
    connection: bullmqConnection,
    removeOnComplete: {
      age: minutesToSeconds(10),
    },
    removeOnFail: {
      count: 1000,
    }
  });
}

export async function cleanPollMasterServerQueue() {
  await getQueuePollMasterServer().obliterate({
    force: true,
  });
}

export async function getLastPollMasterServerDate() {
  return lastCompletedJobDate(getQueuePollMasterServer());
}
