import { minutesToMilliseconds } from "date-fns";
import { schedule } from "../utils";
import { scheduleFillClanActivePlayerCount } from "@teerank/teerank";

export async function fillClanActivePlayerCountScheduler() {
  schedule(minutesToMilliseconds(5), async () => {
    await scheduleFillClanActivePlayerCount();
  });
}
