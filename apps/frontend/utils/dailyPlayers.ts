import { unstable_cache } from 'next/cache';
import prisma from './prisma';
import { addUtcDays, formatUtcDay, utcYesterday } from '@teerank/teerank/date';

export type DailyPlayersPayload = {
  range: string;
  from: string;
  to: string;
  days: { day: string; players: number }[];
};

const MAX_SPAN_DAYS = 5 * 366;

const PRESET_DAYS: Record<string, number> = {
  '30d': 30,
  '90d': 90,
  '1y': 365,
};

export const CACHE_SECONDS = 3600;

export const getDailyPlayers = unstable_cache(
  async (range: string): Promise<DailyPlayersPayload> => {
    const to = utcYesterday();
    let from: Date;

    if (range === 'all') {
      const result = await prisma.globalDay.aggregate({ _min: { day: true } });
      const minFrom = addUtcDays(to, -(MAX_SPAN_DAYS - 1));
      const minDay = result._min.day;
      from = minDay === null || minDay > to ? to : minDay < minFrom ? minFrom : minDay;
    } else {
      range = PRESET_DAYS[range] !== undefined ? range : '90d';
      from = addUtcDays(to, -(PRESET_DAYS[range] - 1));
    }

    const rows = await prisma.globalDay.findMany({
      where: { day: { gte: from, lte: to } },
      select: { day: true, playerCount: true },
      orderBy: { day: 'asc' },
    });

    return {
      range,
      from: formatUtcDay(from),
      to: formatUtcDay(to),
      days: rows.map(({ day, playerCount }) => ({
        day: formatUtcDay(day),
        players: playerCount,
      })),
    };
  },
  ['home-daily-players'],
  { revalidate: CACHE_SECONDS }
);
