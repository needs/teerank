import { masterServerScheduler } from './schedulers/masterServerScheduler';
import { gameServerScheduler } from './schedulers/gameServerScheduler';
import { gameTypeScheduler } from './schedulers/gameTypeScheduler';
import { addDefaultGameTypes } from './addDefaultGameTypes';
import { addDefaultMasterServers } from './addDefaultMasterServers';
import { fillClanActivePlayerCountScheduler } from './schedulers/fillClanActivePlayerCountScheduler';
import { cleanAllQueues } from '@teerank/teerank';
import { updateGlobalCountsScheduler } from './schedulers/updateGlobalCountsScheduler';

async function main() {
  if (process.env.NODE_ENV === 'development') {
    console.log('Cleaning all queues');
    await cleanAllQueues();
  }

  await addDefaultGameTypes();
  await addDefaultMasterServers();

  masterServerScheduler();
  gameServerScheduler();
  gameTypeScheduler();
  fillClanActivePlayerCountScheduler();
  updateGlobalCountsScheduler();
}

main();
