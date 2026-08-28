import { chunk } from "lodash";
import {
  refreshAllDdnetPoints,
  refreshDdnetMapStats,
  refreshDdnetPoints,
  refreshDdnetPointsForMaps,
  refreshDdnetPointsRanks,
} from "@prisma/client/sql";
import { prisma } from "../prisma";

const REFRESH_CHUNK_SIZE = 50_000;

export async function refreshPointsForPlayers(playerIds: number[]) {
  for (const ids of chunk(playerIds, REFRESH_CHUNK_SIZE)) {
    await prisma.$queryRawTyped(refreshDdnetPoints(ids));
  }
}

export async function refreshPointsForMaps(mapIds: number[]) {
  if (mapIds.length > 0) {
    await prisma.$queryRawTyped(refreshDdnetPointsForMaps(mapIds));
  }
}

export async function refreshAllPoints() {
  await prisma.$queryRawTyped(refreshAllDdnetPoints());
}

export async function refreshPointsRanks() {
  await prisma.$queryRawTyped(refreshDdnetPointsRanks());
}

export async function refreshMapStats(mapIds: number[]) {
  for (const ids of chunk(mapIds, 500)) {
    await prisma.$queryRawTyped(refreshDdnetMapStats(ids));
  }
}

export async function refreshAllMapStats() {
  const maps = await prisma.ddnetMap.findMany({ select: { mapId: true } });
  await refreshMapStats(maps.map((map) => map.mapId));
}
