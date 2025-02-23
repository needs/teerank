import { minutesToMilliseconds } from "date-fns";
import { schedule } from "../utils";
import { scheduleUpdateGlobalCounts } from "@teerank/teerank";

export async function updateGlobalCountsScheduler() {
  schedule(minutesToMilliseconds(1), async () => {
    await scheduleUpdateGlobalCounts();
  });
}
