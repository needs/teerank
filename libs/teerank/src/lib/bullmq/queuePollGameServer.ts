import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection, lastCompletedJobDate } from "./config";
import { z } from "zod";
import { minutesToSeconds } from "date-fns";
import { getEnvInt } from "../utils";

let pollGameServerQueue: Queue | null = null;

const QUEUE_NAME_POLL_GAME_SERVER = 'poll-game-server';
const POLL_GAME_SERVER_CONCURRENCY = getEnvInt('POLL_GAME_SERVER_CONCURRENCY', 100);

function getQueuePollGameServer() {
  pollGameServerQueue ??= new Queue(QUEUE_NAME_POLL_GAME_SERVER, { connection: bullmqConnection });
  return pollGameServerQueue;
}

const schema = z.object({
  ip: z.string().optional(),
  port: z.number().optional(),
  gameServerId: z.number().optional(),
});

export type PollGameServerJobData = z.infer<typeof schema>;

export async function schedulePollGameServer(data: PollGameServerJobData) {
  const queue = getQueuePollGameServer();

  await queue.add(
    `${data.ip} - ${data.port} (${data.gameServerId})`,
    {
      ip: data.ip,
      port: data.port,
      gameServerId: data.gameServerId,
    },
    {
      deduplication: {
        id: data.gameServerId ? data.gameServerId.toString() : `${data.ip}:${data.port}`,
      }
    }
  )
}

export async function processPollGameServerJobs(processor: (data: PollGameServerJobData) => Promise<void>) {
  const jobProcessor = async (job: Job) => {
    const data = schema.parse(job.data);
    await processor(data);
  }

  return new Worker(QUEUE_NAME_POLL_GAME_SERVER, jobProcessor, {
    connection: bullmqConnection,
    concurrency: POLL_GAME_SERVER_CONCURRENCY,
    removeOnComplete: {
      age: minutesToSeconds(10),
    },
    removeOnFail: {
      count: 1000,
    }
  });
}

export async function cleanPollGameServerQueue() {
  await getQueuePollGameServer().obliterate({
    force: true,
  });
}

export async function getLastPollGameServerDate() {
  return lastCompletedJobDate(getQueuePollGameServer());
}
