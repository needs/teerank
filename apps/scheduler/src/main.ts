import { scheduleGameServersPolling } from "./schedulers/scheduleGameServersPolling";
import { scheduleMasterServersPolling } from "./schedulers/scheduleMasterServersPolling";
import { schedulePlayerRanking } from "./schedulers/schedulePlayerRanking";
import { scheduleUpdatePlaytimes } from "./schedulers/scheduleUpdatePlaytimes";
import { secondsToMilliseconds } from 'date-fns';
import { addDefaultGameTypes } from './addDefaultGameTypes';
import { addDefaultMasterServers } from './addDefaultMasterServers';

async function main() {
  await addDefaultGameTypes();
  await addDefaultMasterServers();

  let isRunning = false;

  const callback = async () => {
    if (!isRunning) {
      isRunning = true;
      await scheduleMasterServersPolling();
      await scheduleGameServersPolling();
      await schedulePlayerRanking();
      await scheduleUpdatePlaytimes();
      isRunning = false;
    } else {
      console.warn('Skipping a new run because one is already in progress');
    }
  };

  callback();
  setInterval(callback, secondsToMilliseconds(5));
}

main();
