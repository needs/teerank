import { startUpdateMapsCountsWorker } from "./workers/updateMapsCounts";
import { startPollMasterServerWorker } from "./workers/pollMasterServer";
import { startPollGameServerWorker } from "./workers/pollGameServer";
import { startUpdatePlayTimeWorker } from "./workers/updatePlayTime";
import { startRankPlayerWorker } from "./workers/rankPlayer";
import { startUpdateGameTypesCountsWorker } from "./workers/updateGameTypesCounts";
import { startFillClanActivePlayerCountWorker } from "./workers/fillClanActivePlayerCount";
import { startUpdateGlobalCountsWorker } from "./workers/updateGlobalCounts";
import { startArchiveSnapshotsWorker } from "./workers/archiveSnapshots";

async function main() {
  const workers = await Promise.all([
    startPollMasterServerWorker(),
    startPollGameServerWorker(),
    startRankPlayerWorker(),
    startUpdatePlayTimeWorker(),
    startUpdateGameTypesCountsWorker(),
    startUpdateMapsCountsWorker(),
    startFillClanActivePlayerCountWorker(),
    startUpdateGlobalCountsWorker(),
    startArchiveSnapshotsWorker(),
  ]);

  async function gracefulShutdown(signal: string) {
    console.log(`(${signal}) Stopping gracefully...`);
    await Promise.all(workers.map(worker => worker.close()));
    console.log('Stopped gracefully');
    process.exit(0);
  }

  process.on('SIGINT', () => gracefulShutdown('SIGINT'));
  process.on('SIGTERM', () => gracefulShutdown('SIGTERM'));

  console.log('Worker started');
}

main()
