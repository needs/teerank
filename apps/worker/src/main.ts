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
        foundTasks ||= await task();
      } catch (error) {
        console.error(error);
      }
    }

    if (!foundTasks) {
      await wait(5000);
    }
  }
}

main()
