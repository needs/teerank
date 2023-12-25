import { addDefaultGameTypes } from "./tasks/addDefaultGameTypes";
import { addDefaultMasterServers } from "./tasks/addDefaultMasterServers";
import { cleanStuckJobs } from "./tasks/cleanStuckJobs";
import { pollGameServers } from "./tasks/pollGameServers";
import { pollMasterServers } from "./tasks/pollMasterServers";
import { rankPlayers } from "./tasks/rankPlayers";
import { updatePlayTimes } from "./tasks/updatePlayTime";
import { wait } from "./utils";

async function main() {
  await addDefaultGameTypes();
  await addDefaultMasterServers();

  for (; ;) {
    let foundTasks = false;

    for (const task of [cleanStuckJobs, pollMasterServers, pollGameServers, rankPlayers, updatePlayTimes]) {
      try {
        console.time(task.name);
        const hasRun = await task();
        foundTasks ||= hasRun;
        console.timeEnd(task.name);
      } catch (error) {
        console.error(error);
      }
    }

    if (!foundTasks) {
      console.log('No tasks found, waiting 5 seconds');
      await wait(5000);
    }

  }
}

main()
