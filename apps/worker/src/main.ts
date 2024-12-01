import { addDefaultGameTypes } from "./tasks/addDefaultGameTypes";
import { addDefaultMasterServers } from "./tasks/addDefaultMasterServers";
import { reportPerformances, monitorJobPerformance } from "./tasks/reportPerformances";
import { cancellableWait } from "./utils";
import { updateGameTypesCounts } from "./tasks/updateGameTypesCounts";
import { updateMapsCounts } from "./tasks/updateMapsCounts";
import { startPollMasterServerWorker } from "./workers/pollMasterServer";
import { startPollGameServerWorker } from "./workers/pollGameServer";
import { startUpdatePlayTimeWorker } from "./workers/updatePlayTime";
import { startRankPlayerWorker } from "./workers/rankPlayer";

let stopGracefully = false;
const cancellableWaits = new Set<() => void>();

function cancelGracefully() {
  cancellableWaits.forEach(cancel => cancel());
}

process.on('SIGINT', () => {
  console.log('(SIGINT) Stopping gracefully...');
  stopGracefully = true;
  cancelGracefully();
});

async function runJob(job: () => Promise<boolean>, jobName: string, delayOnBusy: number, delayOnIdle: number) {
  console.log(`Starting ${jobName}`);

  while (!stopGracefully) {
    const isBusy = await monitorJobPerformance(jobName, job);

    const { wait, cancel } = cancellableWait(isBusy ? delayOnBusy : delayOnIdle);

    cancellableWaits.add(cancel);
    if (!stopGracefully) {
      await wait;
    }
    cancellableWaits.delete(cancel);
  }

  console.log(`Stopped ${jobName} gracefully`);
}

async function main() {
  await addDefaultGameTypes();
  await addDefaultMasterServers();

  await startPollMasterServerWorker();
  await startPollGameServerWorker();
  await startRankPlayerWorker();
  await startUpdatePlayTimeWorker();

  await Promise.all([
    runJob(updateGameTypesCounts, 'updateGameTypesCounts', 0, 5 * 1000),
    runJob(updateMapsCounts, 'updateMapsCounts', 0, 5 * 1000),
    runJob(reportPerformances, 'reportPerformances', 60000, 60000),
  ]);

  console.log('Stopped gracefully');
}

main()
