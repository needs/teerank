import { minutesToMilliseconds } from "date-fns";
import { scheduleRollupBackfill } from "@teerank/teerank";
import { schedule } from "../utils";

export function rollupBackfillScheduler() {
  schedule(minutesToMilliseconds(15), async () => {
    await scheduleRollupBackfill();
  });
}
