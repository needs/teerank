import { addDefaultGameTypes } from "./tasks/addDefaultGameTypes";
import { addDefaultMasterServers } from "./tasks/addDefaultMasterServers";
import { cleanStuckJobs } from "./tasks/cleanStuckJobs";
import { pollGameServers } from "./tasks/pollGameServers";
import { pollMasterServers } from "./tasks/pollMasterServers";
import { rankPlayers } from "./tasks/rankPlayers";
import { updatePlayTimes } from "./tasks/updatePlayTime";
import { reportPerformances, monitorJobPerformance } from "./tasks/reportPerformances";
import { cancellableWait } from "./utils";
import { removeEmptySnapshots } from "./tasks/removeEmptySnapshots";

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

  await Promise.all([
    runJob(cleanStuckJobs, 'cleanStuckJobs', 5000, 60000),
    runJob(pollMasterServers, 'pollMasterServers', 100, 60000),
    runJob(pollGameServers, 'pollGameServers', 100, 5000),
    runJob(rankPlayers, 'rankPlayers', 0, 5000),
    runJob(updatePlayTimes, 'updatePlayTimes', 0, 5000),
    runJob(removeEmptySnapshots, 'removeEmptySnapshots', 0, 60 * 1000 * 5),
    runJob(reportPerformances, 'reportPerformances', 60000, 60000),
  ]);

  console.log('Stopped gracefully');
}

main()
