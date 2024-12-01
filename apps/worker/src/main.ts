import { addDefaultGameTypes } from "./addDefaultGameTypes";
import { addDefaultMasterServers } from "./addDefaultMasterServers";
import { startUpdateMapsCountsWorker } from "./workers/updateMapsCounts";
import { startPollMasterServerWorker } from "./workers/pollMasterServer";
import { startPollGameServerWorker } from "./workers/pollGameServer";
import { startUpdatePlayTimeWorker } from "./workers/updatePlayTime";
import { startRankPlayerWorker } from "./workers/rankPlayer";
import { startUpdateGameTypesCountsWorker } from "./workers/updateGameTypesCounts";

async function main() {
  await addDefaultGameTypes();
  await addDefaultMasterServers();

  const workers = await Promise.all([
    startPollMasterServerWorker(),
    startPollGameServerWorker(),
    startRankPlayerWorker(),
    startUpdatePlayTimeWorker(),
    startUpdateGameTypesCountsWorker(),
    startUpdateMapsCountsWorker()
  ]);

  process.on('SIGINT', async () => {
    console.log('(SIGINT) Stopping gracefully...');
    await Promise.all(workers.map(worker => worker.close()));
    console.log('Stopped gracefully');
  });
}

main()
