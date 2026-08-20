import { ChartPoint } from '../components/Chart';
import { addUtcDays, utcYesterday } from '@teerank/teerank/date';

export function lastUtcDays(count: number) {
  const yesterday = utcYesterday();

  return Array.from({ length: count }, (_, index) => addUtcDays(yesterday, index - (count - 1)));
}

export function fillSeries(domain: Date[], rows: { at: Date; value: number }[]): ChartPoint[] {
  const values = new Map(rows.map((row) => [row.at.getTime(), row.value]));

  return domain.map((date) => ({
    date,
    value: values.get(date.getTime()) ?? null,
  }));
}
