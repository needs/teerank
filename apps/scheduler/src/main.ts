import { masterServerScheduler } from './schedulers/masterServerScheduler';
import { gameServerScheduler } from './schedulers/gameServerScheduler';
import { gameTypeScheduler } from './schedulers/gameTypeScheduler';
import { addDefaultGameTypes } from './addDefaultGameTypes';
import { addDefaultMasterServers } from './addDefaultMasterServers';
import { fillClanActivePlayerCountScheduler } from './schedulers/fillClanActivePlayerCountScheduler';
import { cleanQueues } from './cleanQueues';
import { updateGlobalCountsScheduler } from './schedulers/updateGlobalCountsScheduler';

cleanQueues().then(() => {
  Promise.all([
    addDefaultGameTypes(),
    addDefaultMasterServers()
  ]).then(() => {
    masterServerScheduler();
    gameServerScheduler();
    gameTypeScheduler();
    fillClanActivePlayerCountScheduler();
    updateGlobalCountsScheduler();
    // Disable map scheduler for now as it consume too much resources
    // mapScheduler();
  });
});
