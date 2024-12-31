import { minutesToMilliseconds } from "date-fns";
import { schedule } from "../utils";
import { getQueueFillClanActivePlayerCount } from "@teerank/teerank";

export async function fillClanActivePlayerCountScheduler() {
  const queue = getQueueFillClanActivePlayerCount();

  schedule(minutesToMilliseconds(1), async () => {
    await queue.add(
      'fill-clan-active-player-count',
      {},
      {
        deduplication: {
          id: 'fill-clan-active-player-count',
        }
      }
    );
  });
}
