import { cleanPollMasterServerQueue } from "./queuePollMasterServer";
import { cleanPollGameServerQueue } from "./queuePollGameServer";
import { cleanGameTypeCountQueue } from "./queueGameTypeCount";
import { cleanMapCountQueue } from "./queueMapCount";
import { cleanFillClanActivePlayerCountQueue } from "./queueFillClanActivePlayerCount";
import { cleanUpdateGlobalCountsQueue } from "./queueUpdateGlobalCounts";
import { cleanUpdatePlayTimeQueue } from "./queueUpdatePlayTime";
import { cleanRankPlayerQueue } from "./queueRankPlayer";

export async function cleanAllQueues() {
  await Promise.all([
    cleanPollMasterServerQueue(),
    cleanPollGameServerQueue(),
    cleanGameTypeCountQueue(),
    cleanMapCountQueue(),
    cleanFillClanActivePlayerCountQueue(),
    cleanUpdateGlobalCountsQueue(),
    cleanUpdatePlayTimeQueue(),
    cleanRankPlayerQueue(),
  ]);
}
