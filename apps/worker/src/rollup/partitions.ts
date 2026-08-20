import { createRollupPartition } from "@prisma/client/sql";
import { prisma } from "../prisma";

const PARTITIONED_TABLES = ["PlayerDay", "ServerDay", "MapDay", "GameTypeDay", "ClanDay"];

function monthStart(date: Date, offsetMonths = 0) {
  return new Date(Date.UTC(date.getUTCFullYear(), date.getUTCMonth() + offsetMonths, 1));
}

export async function ensureRollupPartitions(day: Date) {
  for (const offset of [0, 1]) {
    const from = monthStart(day, offset);
    const to = monthStart(day, offset + 1);

    for (const table of PARTITIONED_TABLES) {
      await prisma.$queryRawTyped(createRollupPartition(table, from, to));
    }
  }
}
