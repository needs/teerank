import { hoursToMilliseconds } from "date-fns";
import { scheduleDdnetOnline } from "@teerank/teerank";
import { schedule } from "../utils";

export function ddnetOnlineScheduler() {
  schedule(hoursToMilliseconds(1), async () => {
    await scheduleDdnetOnline();
  });
}
