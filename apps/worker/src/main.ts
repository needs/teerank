import { addDefaultGameTypes } from "./tasks/addDefaultGameTypes";
import { addDefaultMasterServers } from "./tasks/addDefaultMasterServers";
import { cleanStuckJobs } from "./tasks/cleanStuckJobs";
import { pollGameServers } from "./tasks/pollGameServers";
import { pollMasterServers } from "./tasks/pollMasterServers";
import { rankPlayers } from "./tasks/rankPlayers";
import { updatePlayTimes } from "./tasks/updatePlayTime";
import { wait } from "./utils";

async function runJob(job: () => Promise<boolean>, jobName: string, delayOnBusy: number, delayOnIdle: number) {
  for (; ;) {
    console.time(jobName);
    const isBusy = await job();
    console.timeEnd(jobName);

    if (isBusy) {
      await wait(delayOnBusy);
    } else {
      await wait(delayOnIdle);
    }
  }
}

async function main() {
  await addDefaultGameTypes();
  await addDefaultMasterServers();

  await Promise.all([
    runJob(cleanStuckJobs, 'cleanStuckJobs', 5000, 5000),
    runJob(pollMasterServers, 'pollMasterServers', 0, 60000),
    runJob(pollGameServers, 'pollGameServers', 0, 5000),
    runJob(rankPlayers, 'rankPlayers', 0, 5000),
    runJob(updatePlayTimes, 'updatePlayTimes', 0, 5000),
  ]);
}

main()
