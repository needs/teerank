import {
  cleanQueue,
  getQueueMapCount,
  getQueueGameTypeCount,
  getQueueRankPlayer,
  getQueuePollGameServer,
  getQueuePollMasterServer,
  getQueueUpdatePlayTime
} from '@teerank/teerank';

export async function cleanQueues() {
  await Promise.all([
    cleanQueue(getQueuePollMasterServer()),
    cleanQueue(getQueuePollGameServer()),
    cleanQueue(getQueueUpdatePlayTime()),
    cleanQueue(getQueueRankPlayer()),
    cleanQueue(getQueueGameTypeCount()),
    cleanQueue(getQueueMapCount())
  ]);
}
