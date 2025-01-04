import {
  cleanQueue,
  getQueueMapCount,
  getQueueGameTypeCount,
  getQueuePollGameServer,
  getQueuePollMasterServer,
} from '@teerank/teerank';

export async function cleanQueues() {
  await Promise.all([
    cleanQueue(getQueuePollMasterServer()),
    cleanQueue(getQueuePollGameServer()),
    cleanQueue(getQueueGameTypeCount()),
    cleanQueue(getQueueMapCount())
  ]);
}
