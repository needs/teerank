import { Prisma } from "@prisma/client";
import { prisma } from "../prisma";

export const BACKFILL_STATE_KEY = 'backfill';
export const IMPORT_STATE_KEY = 'import';

// The watermark boundary set covers rows close enough to the dump cut that
// the next dump may still add neighbours with equal-or-older timestamps.
export const BOUNDARY_WINDOW_MS = 10 * 60 * 1000;

export type ImportState = {
  lastModified: string;
  raceWatermark: string;
  raceBoundary: string[];
  teamWatermark: string;
  teamBoundary: string[];
  lastFullRefresh: string;
};

export async function getDdnetState<T>(key: string): Promise<T | null> {
  const row = await prisma.ddnetState.findUnique({ where: { key } });
  return row === null ? null : (row.value as T);
}

export async function setDdnetState(
  tx: Prisma.TransactionClient,
  key: string,
  value: unknown
) {
  await tx.ddnetState.upsert({
    where: { key },
    create: { key, value: value as Prisma.InputJsonValue },
    update: { value: value as Prisma.InputJsonValue },
  });
}

export function monthLabel(date: Date) {
  return date.toISOString().slice(0, 7);
}

export function raceRowKey(row: { mapName: string; playerName: string; timestamp: Date; time: number }) {
  return `${row.mapName}|${row.playerName}|${row.timestamp.toISOString()}|${row.time}`;
}

export class BoundaryTracker {
  private entries = new Map<string, number>();
  private maxMs = 0;

  add(key: string, timestamp: Date) {
    const ms = timestamp.getTime();
    if (ms > this.maxMs) {
      this.maxMs = ms;
      if (this.entries.size > 10_000) {
        this.prune();
      }
    }
    if (ms >= this.maxMs - BOUNDARY_WINDOW_MS) {
      this.entries.set(key, ms);
    }
  }

  private prune() {
    for (const [key, ms] of this.entries) {
      if (ms < this.maxMs - BOUNDARY_WINDOW_MS) {
        this.entries.delete(key);
      }
    }
  }

  finalize() {
    this.prune();
    return {
      watermark: new Date(this.maxMs).toISOString(),
      boundary: [...this.entries.keys()],
    };
  }
}
