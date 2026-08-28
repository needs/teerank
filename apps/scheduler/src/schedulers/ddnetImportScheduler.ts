import { hoursToMilliseconds } from "date-fns";
import { scheduleDdnetImport } from "@teerank/teerank";
import { schedule } from "../utils";

export function ddnetImportScheduler() {
  schedule(hoursToMilliseconds(1), async () => {
    await scheduleDdnetImport();
  });
}
