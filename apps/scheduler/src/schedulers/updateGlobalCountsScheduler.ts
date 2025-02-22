import { minutesToMilliseconds } from "date-fns";
import { schedule } from "../utils";
import { getQueueUpdateGlobalCounts, QUEUE_NAME_UPDATE_GLOBAL_COUNTS } from "@teerank/teerank";

export async function updateGlobalCountsScheduler() {
  const queue = getQueueUpdateGlobalCounts();

  schedule(minutesToMilliseconds(1), async () => {
    await queue.add(
      QUEUE_NAME_UPDATE_GLOBAL_COUNTS,
      {},
      {
        deduplication: {
          id: 'update-global-counts',
        }
      }
    );
  });
}
