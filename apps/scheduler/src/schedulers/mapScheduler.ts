import { scheduleMapCount } from "@teerank/teerank";
import { hoursToMilliseconds } from "date-fns";
import { schedule } from "../utils";

export async function mapScheduler() {
  schedule(hoursToMilliseconds(24), async () => {
    await scheduleMapCount({ mode: 'full' });
  });

  schedule(hoursToMilliseconds(1), async () => {
    await scheduleMapCount({ mode: 'gameServers' });
  });
}
