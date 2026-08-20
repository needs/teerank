import { hoursToMilliseconds } from "date-fns";
import { schedule } from "../utils";
import { addUtcDays, formatUtcDay, scheduleRollupDay } from "@teerank/teerank";

export function rollupDayScheduler() {
  schedule(hoursToMilliseconds(1), async () => {
    await scheduleRollupDay({ day: formatUtcDay(addUtcDays(new Date(), -1)) });
  });
}
