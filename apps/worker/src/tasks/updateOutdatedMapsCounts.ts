import { prisma } from "../prisma";
import { nextMapToCount, updateMapCounts } from "@teerank/teerank"

export async function updateOutdatedMapsCounts() {
  const map = await nextMapToCount(prisma);

  if (map === null) {
    return false;
  }

  await updateMapCounts(prisma, map);
  return true;
}
