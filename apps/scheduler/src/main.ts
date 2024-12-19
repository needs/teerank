import { masterServerScheduler } from './schedulers/masterServerScheduler';
import { gameServerScheduler } from './schedulers/gameServerScheduler';
import { gameTypeScheduler } from './schedulers/gameTypeScheduler';
import { mapScheduler } from './schedulers/mapScheduler';
import { addDefaultGameTypes } from './addDefaultGameTypes';
import { addDefaultMasterServers } from './addDefaultMasterServers';
import { cleanQueues } from './cleanQueues';

cleanQueues().then(() => {
  Promise.all([
    addDefaultGameTypes(),
    addDefaultMasterServers()
  ]).then(() => {
    masterServerScheduler();
    gameServerScheduler();
    gameTypeScheduler();
    mapScheduler();
  });
});
