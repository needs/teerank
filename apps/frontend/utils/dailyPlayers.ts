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

export const getDailyPlayers = unstable_cache(
  async (range: string): Promise<DailyPlayersPayload> => {
    const to = utcYesterday();
    let from: Date;

    if (range === 'all') {
      const result = await prisma.playerDay.aggregate({ _min: { day: true } });
      const minFrom = addUtcDays(to, -(MAX_SPAN_DAYS - 1));
      const minDay = result._min.day;
      from = minDay === null || minDay > to ? to : minDay < minFrom ? minFrom : minDay;
    } else {
      range = PRESET_DAYS[range] !== undefined ? range : '90d';
      from = addUtcDays(to, -(PRESET_DAYS[range] - 1));
    }

    const rows = await prisma.playerDay.groupBy({
      by: ['day'],
      where: { day: { gte: from } },
      _count: { _all: true },
      orderBy: { day: 'asc' },
    });

    return {
      range,
      from: formatUtcDay(from),
      to: formatUtcDay(to),
      days: rows.map(({ day, _count }) => ({ day: formatUtcDay(day), players: _count._all })),
    };
  },
  ['home-daily-players'],
  { revalidate: 3600 }
);
