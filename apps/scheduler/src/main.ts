import { scheduleGameServersPolling } from "./schedulers/scheduleGameServersPolling";
import { scheduleMasterServersPolling } from "./schedulers/scheduleMasterServersPolling";
import { schedulePlayerRanking } from "./schedulers/schedulePlayerRanking";
import { scheduleUpdatePlaytimes } from "./schedulers/scheduleUpdatePlaytimes";

async function main() {
  setInterval(async () => {
    await scheduleMasterServersPolling();
    await scheduleGameServersPolling();
    await schedulePlayerRanking();
    await scheduleUpdatePlaytimes();
  }, 1000 * 60);
}

main();
