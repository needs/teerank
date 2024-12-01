import { masterServerScheduler } from './schedulers/masterServerScheduler';
import { gameServerScheduler } from './schedulers/gameServerScheduler';
import { gameTypeScheduler } from './schedulers/gameTypeScheduler';
import { mapScheduler } from './schedulers/mapScheduler';

masterServerScheduler();
gameServerScheduler();
gameTypeScheduler();
mapScheduler();
