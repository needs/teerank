import { Job, Queue, Worker } from "bullmq";
import { bullmqConnection, lastCompletedJobDate } from "./config";
import { z } from "zod";
import { hoursToSeconds } from "date-fns";

let archiveSnapshotsQueue: Queue | null = null;

const QUEUE_NAME_ARCHIVE_SNAPSHOTS = 'archive-snapshots';

function getQueueArchiveSnapshots() {
  archiveSnapshotsQueue ??= new Queue(QUEUE_NAME_ARCHIVE_SNAPSHOTS, { connection: bullmqConnection });
  return archiveSnapshotsQueue;
}

const schema = z.object({});

export type ArchiveSnapshotsJobData = z.infer<typeof schema>;

export async function scheduleArchiveSnapshots() {
  const queue = getQueueArchiveSnapshots();
  await queue.add('archive-snapshots', {}, {
    deduplication: {
      id: 'archive-snapshots',
    }
  });
}

export async function processArchiveSnapshotsJobs(processor: (data: ArchiveSnapshotsJobData) => Promise<void>) {
  const jobProcessor = async (job: Job) => {
    const data = schema.parse(job.data);
    await processor(data);
  }

  return new Worker(QUEUE_NAME_ARCHIVE_SNAPSHOTS, jobProcessor, {
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

export async function cleanArchiveSnapshotsQueue() {
  await getQueueArchiveSnapshots().obliterate({
    force: true,
  });
}

export async function getLastArchiveSnapshotsDate() {
  return lastCompletedJobDate(getQueueArchiveSnapshots());
}

export async function getArchiveSnapshotsFailedCount() {
  return getQueueArchiveSnapshots().getFailedCount();
}

export { getQueueArchiveSnapshots };
