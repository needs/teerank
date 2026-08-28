import { cleanPollMasterServerQueue } from "./queuePollMasterServer";
import { cleanPollGameServerQueue } from "./queuePollGameServer";
import { cleanGameTypeCountQueue } from "./queueGameTypeCount";
import { cleanMapCountQueue } from "./queueMapCount";
import { cleanFillClanActivePlayerCountQueue } from "./queueFillClanActivePlayerCount";
import { cleanUpdateGlobalCountsQueue } from "./queueUpdateGlobalCounts";
import { cleanUpdatePlayTimeQueue } from "./queueUpdatePlayTime";
import { cleanRankPlayerQueue } from "./queueRankPlayer";
import { cleanArchiveSnapshotsQueue } from "./queueArchiveSnapshots";
import { cleanRollupDayQueue } from "./queueRollupDay";
import { cleanRollupBackfillQueue } from "./queueRollupBackfill";
import { cleanDdnetBackfillQueue } from "./queueDdnetBackfill";
import { cleanDdnetImportQueue } from "./queueDdnetImport";
import { cleanDdnetOnlineQueue } from "./queueDdnetOnline";

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
    cleanArchiveSnapshotsQueue(),
    cleanRollupDayQueue(),
    cleanRollupBackfillQueue(),
    cleanDdnetBackfillQueue(),
    cleanDdnetImportQueue(),
    cleanDdnetOnlineQueue(),
  ]);
}
