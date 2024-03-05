import { prisma } from "../prisma";
import { nextGameTypeToCount, updateGameTypeCountsIfOutdated } from "@teerank/teerank"

export async function updateGameTypesCounts() {
  const gameType = await nextGameTypeToCount(prisma);

  if (gameType === null) {
    return false;
  }

  await updateGameTypeCountsIfOutdated(prisma, gameType);
  return true;
}
