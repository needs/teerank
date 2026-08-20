import { format } from 'date-fns';
import prisma from './prisma';
import { formatInteger, formatPlayTime } from './format';
import { DAY_MS, addUtcDays, formatUtcDay, parseUtcDay, utcYesterday } from '@teerank/teerank/date';
import type { ActivityDay, ActivityPayload } from '../components/ActivityCalendar';

export type RangeParams = {
  range?: string;
  from?: string;
  to?: string;
};

const MAX_SPAN_DAYS = 5 * 366;

const PRESET_DAYS: Record<string, number> = {
  '30d': 30,
  '90d': 90,
  '1y': 365,
};

export function rangeParamsFromSearch(searchParams: URLSearchParams): RangeParams {
  return {
    range: searchParams.get('range') ?? undefined,
    from: searchParams.get('from') ?? undefined,
    to: searchParams.get('to') ?? undefined,
  };
}

function parseDayParam(day: string | undefined) {
  if (day === undefined || !/^\d{4}-\d{2}-\d{2}$/.test(day)) {
    return null;
  }

  const date = parseUtcDay(day);
  return Number.isNaN(date.getTime()) ? null : date;
}

async function resolveDomain(
  params: RangeParams,
  getMinDay: () => Promise<Date | null>
): Promise<{ range: string; from: Date; to: Date }> {
  const yesterday = utcYesterday();
  const customFrom = parseDayParam(params.from);
  const customTo = parseDayParam(params.to);

  if (customFrom !== null && customTo !== null && customFrom <= customTo) {
    const to = customTo > yesterday ? yesterday : customTo;
    const minFrom = addUtcDays(to, -(MAX_SPAN_DAYS - 1));
    const from = customFrom < minFrom ? minFrom : customFrom;
    return { range: 'custom', from, to };
  }

  if (params.range === 'all') {
    const minDay = await getMinDay();
    const minFrom = addUtcDays(yesterday, -(MAX_SPAN_DAYS - 1));
    const from =
      minDay === null || minDay > yesterday
        ? yesterday
        : new Date(Math.floor(minDay.getTime() / DAY_MS) * DAY_MS);
    return { range: 'all', from: from < minFrom ? minFrom : from, to: yesterday };
  }

  const days = PRESET_DAYS[params.range ?? '90d'] ?? 90;
  return {
    range: PRESET_DAYS[params.range ?? ''] !== undefined ? (params.range as string) : '90d',
    from: addUtcDays(yesterday, -(days - 1)),
    to: yesterday,
  };
}

function payload(range: string, from: Date, to: Date, days: ActivityDay[]): ActivityPayload {
  return { range, from: formatUtcDay(from), to: formatUtcDay(to), days };
}

function tooltipDate(day: Date) {
  return format(day, 'MMM d, yyyy');
}

export async function getPlayerActivity(playerId: number, params: RangeParams) {
  const { range, from, to } = await resolveDomain(params, async () => {
    const result = await prisma.playerDay.aggregate({
      _min: { day: true },
      where: { playerId },
    });
    return result._min.day;
  });

  const rows = await prisma.playerDay.findMany({
    where: { playerId, day: { gte: from, lte: to } },
    select: { day: true, playTime: true },
  });

  return payload(range, from, to, rows.map((row) => ({
    day: formatUtcDay(row.day),
    value: row.playTime,
    tooltip: `${formatPlayTime(BigInt(row.playTime))} on ${tooltipDate(row.day)}`,
  })));
}

export async function getClanActivity(clanId: number, params: RangeParams) {
  const { range, from, to } = await resolveDomain(params, async () => {
    const result = await prisma.clanDay.aggregate({
      _min: { day: true },
      where: { clanId },
    });
    return result._min.day;
  });

  const rows = await prisma.clanDay.findMany({
    where: { clanId, day: { gte: from, lte: to } },
    select: { day: true, playTime: true, playerCount: true },
  });

  return payload(range, from, to, rows.map((row) => ({
    day: formatUtcDay(row.day),
    value: row.playTime,
    tooltip: `${formatPlayTime(BigInt(row.playTime))} · ${formatInteger(row.playerCount)} players on ${tooltipDate(row.day)}`,
  })));
}

export async function getGameTypeActivity(gameTypeId: number, params: RangeParams) {
  const { range, from, to } = await resolveDomain(params, async () => {
    const result = await prisma.gameTypeDay.aggregate({
      _min: { day: true },
      where: { gameTypeId },
    });
    return result._min.day;
  });

  const rows = await prisma.gameTypeDay.findMany({
    where: { gameTypeId, day: { gte: from, lte: to } },
    select: { day: true, playTime: true, playerCount: true },
  });

  return payload(range, from, to, rows.map((row) => ({
    day: formatUtcDay(row.day),
    value: row.playTime,
    tooltip: `${formatPlayTime(BigInt(row.playTime))} · ${formatInteger(row.playerCount)} players on ${tooltipDate(row.day)}`,
  })));
}

export async function getMapActivity(mapId: number, params: RangeParams) {
  const { range, from, to } = await resolveDomain(params, async () => {
    const result = await prisma.mapDay.aggregate({
      _min: { day: true },
      where: { mapId },
    });
    return result._min.day;
  });

  const rows = await prisma.mapDay.findMany({
    where: { mapId, day: { gte: from, lte: to } },
    select: { day: true, playTime: true, playerCount: true },
  });

  return payload(range, from, to, rows.map((row) => ({
    day: formatUtcDay(row.day),
    value: row.playTime,
    tooltip: `${formatPlayTime(BigInt(row.playTime))} · ${formatInteger(row.playerCount)} players on ${tooltipDate(row.day)}`,
  })));
}

export async function getServerActivity(gameServerId: number, params: RangeParams) {
  const { range, from, to } = await resolveDomain(params, async () => {
    const result = await prisma.serverDay.aggregate({
      _min: { day: true },
      where: { gameServerId },
    });
    return result._min.day;
  });

  const rows = await prisma.serverDay.findMany({
    where: { gameServerId, day: { gte: from, lte: to } },
    select: { day: true, avgClients: true, maxClients: true },
  });

  return payload(range, from, to, rows.map((row) => ({
    day: formatUtcDay(row.day),
    value: row.avgClients,
    tooltip: `${row.avgClients} avg clients · peak ${row.maxClients} on ${tooltipDate(row.day)}`,
  })));
}
