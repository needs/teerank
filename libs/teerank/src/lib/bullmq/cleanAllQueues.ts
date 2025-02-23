import { Queue } from "bullmq";
import { cleanPollMasterServerQueue } from "./queuePollMasterServer";
import { cleanPollGameServerQueue } from "./queuePollGameServer";
import { getQueueGameTypeCount } from "./queueGameTypeCount";
import { getQueueMapCount } from "./queueMapCount";
import { getQueueFillClanActivePlayerCount } from "./queueFillClanActivePlayerCount";
import { getQueueUpdateGlobalCounts } from "./queueUpdateGlobalCounts";
import { getQueueUpdatePlayTime } from "./queueUpdatePlayTime";
import { getQueueRankPlayer } from "./queueRankPlayer";

export async function cleanQueue(queue: Queue) {
  console.log('Cleaning queue', queue.name);
  await queue.obliterate({
    force: true,
  });
  console.log('Queue cleaned', queue.name);
}

export async function cleanAllQueues() {
  await Promise.all([
    cleanPollMasterServerQueue(),
    cleanPollGameServerQueue(),
    cleanQueue(getQueueGameTypeCount()),
    cleanQueue(getQueueMapCount()),
    cleanQueue(getQueueFillClanActivePlayerCount()),
    cleanQueue(getQueueUpdateGlobalCounts()),
    cleanQueue(getQueueUpdatePlayTime()),
    cleanQueue(getQueueRankPlayer()),
  ]);
}
