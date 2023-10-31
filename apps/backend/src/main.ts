import { addDefaultMasterServers } from './addDefaultMasterServers';
import { pollGameServers } from './pollGameServers';
import { pollMasterServers } from './pollMasterServers';
import { rankPlayers } from './rankPlayers';
import { updateGameTypesRankMethod } from './updateGameTypesRankMethod';

async function main() {
  await addDefaultMasterServers();
  await updateGameTypesRankMethod();

  let isRunning = false;

  const callback = async () => {
    if (!isRunning) {
      isRunning = true;
      await pollMasterServers();
      await pollGameServers();
      await rankPlayers();
      isRunning = false;
    } else {
      console.warn('Skipping poll because it is already running');
    }
  };

  callback();
  setInterval(callback, 1000 * 60 * 5);
}

main();
