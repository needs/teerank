import { updateMapsCounts, updateMapsGameServerCounts } from "@prisma/client/sql";
import { prisma } from "../prisma";
import { MapCountJobData, processMapCountJobs } from "@teerank/teerank"

export async function updateMapsCount(data: MapCountJobData) {
  if (data.mode === 'full') {
    await prisma.$queryRawTyped(updateMapsCounts());
  } else {
    await prisma.$queryRawTyped(updateMapsGameServerCounts());
  }
}

export async function startUpdateMapsCountsWorker() {
  return await processMapCountJobs(updateMapsCount);
}
