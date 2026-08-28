import { minutesToMilliseconds } from "date-fns";
import { scheduleDdnetBackfill } from "@teerank/teerank";
import { schedule } from "../utils";

export function ddnetBackfillScheduler() {
  schedule(minutesToMilliseconds(15), async () => {
    await scheduleDdnetBackfill();
  });
}
