import { minutesToMilliseconds } from "date-fns";
import { processDdnetOnlineJobs } from "@teerank/teerank";
import { prisma } from "../prisma";
import { DDNET_STATUS_BASE_URL } from "../ddnet/stream";
import { getDdnetState, setDdnetState } from "../ddnet/state";

const SOURCES = [
  { stateKey: 'online-bycountry', kind: 0, url: `${DDNET_STATUS_BASE_URL}/bycountry`, withTotal: true },
  { stateKey: 'online-bymod', kind: 1, url: `${DDNET_STATUS_BASE_URL}/bymod`, withTotal: false },
];

const FLUSH_DAY_COUNT = 60;

type OnlineState = { offset: number };

type DayAggregate = {
  day: string;
  samples: number;
  byKey: Map<string, { sum: number; max: number }>;
  totalSum: number;
  totalMax: number;
};

function newDayAggregate(day: string): DayAggregate {
  return { day, samples: 0, byKey: new Map(), totalSum: 0, totalMax: 0 };
}

function finalizeDay(aggregate: DayAggregate, kind: number, withTotal: boolean) {
  const day = new Date(`${aggregate.day}T00:00:00Z`);
  const rows: { day: Date; kind: number; key: string; avgPlayers: number; maxPlayers: number }[] = [];

  for (const [key, value] of aggregate.byKey) {
    rows.push({
      day,
      kind,
      key,
      // A key missing from a sample counts as zero, so the divisor is the
      // day's full sample count.
      avgPlayers: Math.round(value.sum / aggregate.samples),
      maxPlayers: value.max,
    });
  }

  if (withTotal) {
    rows.push({
      day,
      kind,
      key: 'ALL',
      avgPlayers: Math.round(aggregate.totalSum / aggregate.samples),
      maxPlayers: aggregate.totalMax,
    });
  }

  return rows;
}

async function flushDays(
  rows: { day: Date; kind: number; key: string; avgPlayers: number; maxPlayers: number }[],
  kind: number,
  stateKey: string,
  offset: number
) {
  const days = [...new Set(rows.map((row) => row.day.getTime()))].map((ms) => new Date(ms));

  await prisma.$transaction(
    async (tx) => {
      await tx.ddnetOnlineDay.deleteMany({ where: { kind, day: { in: days } } });
      await tx.ddnetOnlineDay.createMany({ data: rows });
      await setDdnetState(tx, stateKey, { offset } satisfies OnlineState);
    },
    { timeout: minutesToMilliseconds(5), maxWait: minutesToMilliseconds(1) }
  );
}

async function importSource(source: typeof SOURCES[number]) {
  const state = await getDdnetState<OnlineState>(source.stateKey);
  let offset = state?.offset ?? 0;

  const response = await fetch(source.url, {
    headers: offset > 0 ? { Range: `bytes=${offset}-` } : {},
  });

  if (response.status === 416) {
    return; // Nothing new past the offset.
  }
  if (!response.ok || response.body === null) {
    throw new Error(`GET ${source.url} failed: ${response.status}`);
  }
  if (response.status === 200) {
    offset = 0; // Server ignored the range or the file was rewritten.
  }

  const reader = response.body.getReader();
  const decoder = new TextDecoder();

  let pendingRows: ReturnType<typeof finalizeDay> = [];
  let current: DayAggregate | null = null;
  let currentDayStart = offset;
  let lineStart = offset;
  let buffered = '';

  const handleLine = (line: string) => {
    // "YYYY-MM-DD HH:MM,KEY:count,KEY:count,..."
    const day = line.slice(0, 10);
    if (!/^\d{4}-\d{2}-\d{2}$/.test(day)) {
      return;
    }

    if (current === null || current.day !== day) {
      if (current !== null) {
        pendingRows.push(...finalizeDay(current, source.kind, source.withTotal));
      }
      current = newDayAggregate(day);
      currentDayStart = lineStart;
    }

    current.samples += 1;
    let total = 0;

    const fields = line.split(',');
    for (let index = 1; index < fields.length; index++) {
      const separator = fields[index].lastIndexOf(':');
      if (separator === -1) {
        continue;
      }
      const key = fields[index].slice(0, separator);
      const count = Number(fields[index].slice(separator + 1));
      if (!Number.isFinite(count)) {
        continue;
      }

      total += count;
      const entry = current.byKey.get(key);
      if (entry === undefined) {
        current.byKey.set(key, { sum: count, max: count });
      } else {
        entry.sum += count;
        if (count > entry.max) entry.max = count;
      }
    }

    if (total > current.totalMax) current.totalMax = total;
    current.totalSum += total;
  };

  for (;;) {
    const { done, value } = await reader.read();
    const text = done ? decoder.decode() : decoder.decode(value, { stream: true });
    buffered += text;

    let newlineIndex: number;
    while ((newlineIndex = buffered.indexOf('\n')) !== -1) {
      const line = buffered.slice(0, newlineIndex);
      handleLine(line.trimEnd());
      lineStart += Buffer.byteLength(line, 'utf8') + 1;
      buffered = buffered.slice(newlineIndex + 1);
    }

    if (pendingRows.length >= FLUSH_DAY_COUNT * 10) {
      // The offset stays at the start of the still-open day so the next run
      // re-reads and re-finalizes it.
      await flushDays(pendingRows, source.kind, source.stateKey, currentDayStart);
      pendingRows = [];
    }

    if (done) {
      break;
    }
  }

  // The last (incomplete) day is intentionally not finalized.
  if (pendingRows.length > 0) {
    await flushDays(pendingRows, source.kind, source.stateKey, currentDayStart);
  }
}

export async function ddnetOnlineImport() {
  for (const source of SOURCES) {
    await importSource(source);
  }
}

export async function startDdnetOnlineWorker() {
  return processDdnetOnlineJobs(ddnetOnlineImport);
}
