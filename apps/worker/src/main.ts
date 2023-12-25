import { addDefaultGameTypes } from "./tasks/addDefaultGameTypes";
import { addDefaultMasterServers } from "./tasks/addDefaultMasterServers";
import { cleanStuckJobs } from "./tasks/cleanStuckJobs";
import { pollGameServers } from "./tasks/pollGameServers";
import { pollMasterServers } from "./tasks/pollMasterServers";
import { rankPlayers } from "./tasks/rankPlayers";
import { updatePlayTimes } from "./tasks/updatePlayTime";
import { cancellableWait } from "./utils";

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

process.on('SIGTERM', () => {
  console.log('(SIGTERM) Stopping gracefully...');
  stopGracefully = true;
  cancelGracefully();
});

process.on('exit', () => {
  console.log('(exit) Stopping gracefully...');
  stopGracefully = true;
  cancelGracefully();
});

async function runJob(job: () => Promise<boolean>, jobName: string, delayOnBusy: number, delayOnIdle: number) {
  console.log(`Starting ${jobName}`);

  while (!stopGracefully) {
    console.time(jobName);
    const isBusy = await job();
    console.timeEnd(jobName);

    const {wait, cancel} = cancellableWait(isBusy ? delayOnBusy : delayOnIdle);

    if (!stopGracefully) {
      cancellableWaits.add(cancel);
      await wait;
      cancellableWaits.delete(cancel);
    }
  }

  console.log(`Stopped ${jobName} gracefully`);
}

async function main() {
  await addDefaultGameTypes();
  await addDefaultMasterServers();

  await Promise.all([
    runJob(cleanStuckJobs, 'cleanStuckJobs', 5000, 60000),
    runJob(pollMasterServers, 'pollMasterServers', 0, 60000),
    runJob(pollGameServers, 'pollGameServers', 0, 5000),
    runJob(rankPlayers, 'rankPlayers', 0, 5000),
    runJob(updatePlayTimes, 'updatePlayTimes', 0, 5000),
  ]);

  console.log('Stopped gracefully');
}

main()
