import { prisma } from "../prisma";
import { nextMapToCount, updateMapCountsIfOutdated } from "@teerank/teerank"

export async function updateMapsCounts() {
  const map = await nextMapToCount(prisma);

  if (map === null) {
    return false;
  }

  await updateMapCountsIfOutdated(prisma, map);
  return true;
}
