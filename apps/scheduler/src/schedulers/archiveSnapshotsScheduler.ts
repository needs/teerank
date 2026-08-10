import { minutesToMilliseconds } from "date-fns";
import { schedule } from "../utils";
import { scheduleArchiveSnapshots } from "@teerank/teerank";

export async function archiveSnapshotsScheduler() {
  schedule(minutesToMilliseconds(10), async () => {
    await scheduleArchiveSnapshots();
  });
}
