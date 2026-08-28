import { unstable_cache } from 'next/cache';
import prisma from './prisma';
import { addUtcDays, formatUtcDay, utcYesterday } from '@teerank/teerank/date';

export type DailyPlayersPayload = {
  range: string;
  source: string;
  from: string;
  to: string;
  days?: { day: string; players: number }[];
  keys?: string[];
  stacks?: { day: string; values: number[] }[];
};

const MAX_SPAN_DAYS = 5 * 366;

const DDNET_MAX_SPAN_DAYS = 20 * 366;

const PRESET_DAYS: Record<string, number> = {
  '30d': 30,
  '90d': 90,
  '1y': 365,
};

export const DAILY_PLAYERS_SOURCES = ['teerank', 'ddnet', 'bycountry', 'bymod'] as const;
export type DailyPlayersSource = typeof DAILY_PLAYERS_SOURCES[number];

const TOP_KEY_COUNT = 7;

export const CACHE_SECONDS = 3600;

async function resolveRange(
  range: string,
  getMinDay: () => Promise<Date | null>,
  maxSpanDays: number
) {
  const to = utcYesterday();
  let from: Date;

  if (range === 'all') {
    const minFrom = addUtcDays(to, -(maxSpanDays - 1));
    const minDay = await getMinDay();
    from = minDay === null || minDay > to ? to : minDay < minFrom ? minFrom : minDay;
  } else {
    range = PRESET_DAYS[range] !== undefined ? range : '90d';
    from = addUtcDays(to, -(PRESET_DAYS[range] - 1));
  }

  return { range, from, to };
}

function spanDaysOf(from: Date, to: Date) {
  return (to.getTime() - from.getTime()) / 86_400_000 + 1;
}

function bucketLabel(day: Date, spanDays: number) {
  if (spanDays <= 366) {
    return formatUtcDay(day);
  }
  if (spanDays <= MAX_SPAN_DAYS) {
    const weekStart = addUtcDays(day, -((day.getUTCDay() + 6) % 7));
    return formatUtcDay(weekStart);
  }
  return `${formatUtcDay(day).slice(0, 7)}-01`;
}

async function getTeerankPayload(range: string): Promise<Omit<DailyPlayersPayload, 'source'>> {
  const resolved = await resolveRange(range, async () => {
    const result = await prisma.globalDay.aggregate({ _min: { day: true } });
    return result._min.day;
  }, MAX_SPAN_DAYS);

  const rows = await prisma.globalDay.findMany({
    where: { day: { gte: resolved.from, lte: resolved.to } },
    select: { day: true, playerCount: true },
    orderBy: { day: 'asc' },
  });

  return {
    range: resolved.range,
    from: formatUtcDay(resolved.from),
    to: formatUtcDay(resolved.to),
    days: rows.map(({ day, playerCount }) => ({ day: formatUtcDay(day), players: playerCount })),
  };
}

async function getDdnetMinDay(kind: number) {
  const result = await prisma.ddnetOnlineDay.aggregate({
    _min: { day: true },
    where: { kind },
  });
  return result._min.day;
}

async function getDdnetTotalPayload(range: string): Promise<Omit<DailyPlayersPayload, 'source'>> {
  const resolved = await resolveRange(range, () => getDdnetMinDay(0), DDNET_MAX_SPAN_DAYS);

  const rows = await prisma.ddnetOnlineDay.findMany({
    where: { kind: 0, key: 'ALL', day: { gte: resolved.from, lte: resolved.to } },
    select: { day: true, avgPlayers: true },
    orderBy: { day: 'asc' },
  });

  const spanDays = spanDaysOf(resolved.from, resolved.to);
  const buckets = new Map<string, { sum: number; count: number }>();

  for (const row of rows) {
    const label = bucketLabel(row.day, spanDays);
    const bucket = buckets.get(label) ?? { sum: 0, count: 0 };
    bucket.sum += row.avgPlayers;
    bucket.count += 1;
    buckets.set(label, bucket);
  }

  return {
    range: resolved.range,
    from: formatUtcDay(resolved.from),
    to: formatUtcDay(resolved.to),
    days: [...buckets.entries()].map(([day, { sum, count }]) => ({
      day,
      players: Math.round(sum / count),
    })),
  };
}

async function getDdnetBreakdownPayload(
  range: string,
  kind: number
): Promise<Omit<DailyPlayersPayload, 'source'>> {
  const resolved = await resolveRange(range, () => getDdnetMinDay(kind), DDNET_MAX_SPAN_DAYS);

  const rows = await prisma.ddnetOnlineDay.findMany({
    where: { kind, key: { not: 'ALL' }, day: { gte: resolved.from, lte: resolved.to } },
    select: { day: true, key: true, avgPlayers: true },
    orderBy: { day: 'asc' },
  });

  const totals = new Map<string, number>();
  for (const row of rows) {
    totals.set(row.key, (totals.get(row.key) ?? 0) + row.avgPlayers);
  }

  const topKeys = [...totals.entries()]
    .sort((a, b) => b[1] - a[1])
    .slice(0, TOP_KEY_COUNT)
    .map(([key]) => key);
  const keys = totals.size > topKeys.length ? [...topKeys, 'Other'] : topKeys;
  const keyIndex = new Map(keys.map((key, index) => [key, index]));

  const spanDays = spanDaysOf(resolved.from, resolved.to);
  const buckets = new Map<string, { sums: number[]; days: Set<string> }>();

  for (const row of rows) {
    const label = bucketLabel(row.day, spanDays);
    let bucket = buckets.get(label);
    if (bucket === undefined) {
      bucket = { sums: keys.map(() => 0), days: new Set() };
      buckets.set(label, bucket);
    }
    const index = keyIndex.get(row.key) ?? keys.length - 1;
    bucket.sums[index] += row.avgPlayers;
    bucket.days.add(formatUtcDay(row.day));
  }

  return {
    range: resolved.range,
    from: formatUtcDay(resolved.from),
    to: formatUtcDay(resolved.to),
    keys,
    stacks: [...buckets.entries()].map(([day, bucket]) => ({
      day,
      values: bucket.sums.map((sum) => Math.round(sum / bucket.days.size)),
    })),
  };
}

export const getDailyPlayers = unstable_cache(
  async (range: string, source = 'teerank'): Promise<DailyPlayersPayload> => {
    const source_: DailyPlayersSource = (DAILY_PLAYERS_SOURCES as readonly string[]).includes(source)
      ? (source as DailyPlayersSource)
      : 'teerank';

    switch (source_) {
      case 'ddnet':
        return { source: source_, ...(await getDdnetTotalPayload(range)) };
      case 'bycountry':
        return { source: source_, ...(await getDdnetBreakdownPayload(range, 0)) };
      case 'bymod':
        return { source: source_, ...(await getDdnetBreakdownPayload(range, 1)) };
      default:
        return { source: source_, ...(await getTeerankPayload(range)) };
    }
  },
  ['home-daily-players'],
  { revalidate: CACHE_SECONDS }
);
