import { hoursToMilliseconds } from 'date-fns';

export const DAY_MS = hoursToMilliseconds(24);

export function parseUtcDay(day: string) {
  return new Date(`${day}T00:00:00.000Z`);
}

export function formatUtcDay(date: Date) {
  return date.toISOString().slice(0, 10);
}

export function addUtcDays(date: Date, days: number) {
  return new Date(date.getTime() + days * DAY_MS);
}

export function startOfUtcDay(date: Date) {
  return new Date(Math.floor(date.getTime() / DAY_MS) * DAY_MS);
}

export function utcYesterday() {
  return addUtcDays(startOfUtcDay(new Date()), -1);
}

export function eachUtcDay(from: Date, to: Date) {
  const count = Math.round((to.getTime() - from.getTime()) / DAY_MS) + 1;

  return Array.from({ length: count }, (_, index) => addUtcDays(from, index));
}
