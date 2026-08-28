'use client';

import { useState } from 'react';
import { BarChart, StackedBarChart } from './Chart';
import { fillSeries } from '../utils/series';
import { eachUtcDay, parseUtcDay } from '@teerank/teerank/date';
import { DailyPlayersPayload } from '../utils/dailyPlayers';

const PRESETS: { key: string; label: string }[] = [
  { key: '30d', label: 'Last 30 days' },
  { key: '90d', label: 'Last 90 days' },
  { key: '1y', label: 'Last year' },
  { key: 'all', label: 'All time' },
];

const SOURCES: { key: string; label: string }[] = [
  { key: 'teerank', label: 'Teerank players' },
  { key: 'ddnet', label: 'DDNet total' },
  { key: 'bycountry', label: 'DDNet by country' },
  { key: 'bymod', label: 'DDNet by mod' },
];

function Dropdown({
  options,
  current,
  onSelect,
}: {
  options: { key: string; label: string }[];
  current: string;
  onSelect: (key: string) => void;
}) {
  const [open, setOpen] = useState(false);

  return (
    <span className="relative">
      <button
        onClick={() => setOpen(!open)}
        className="text-md clear-both text-[#888] hover:underline"
      >
        {options.find(({ key }) => key === current)?.label ?? current} ▾
      </button>

      {open && (
        <>
          <span className="fixed inset-0 z-10" onClick={() => setOpen(false)} />
          <span className="absolute right-0 top-full z-20 mt-1 flex w-max flex-col rounded-md border bg-white p-1 shadow-md">
            {options.map((option) => (
              <button
                key={option.key}
                onClick={() => {
                  setOpen(false);
                  onSelect(option.key);
                }}
                className={`rounded px-3 py-1 text-left text-sm hover:bg-[#f4efdc] ${
                  current === option.key ? 'font-bold text-[#970]' : 'text-[#666]'
                }`}
              >
                {option.label}
              </button>
            ))}
          </span>
        </>
      )}
    </span>
  );
}

export function DailyPlayersSection({ initial }: { initial: DailyPlayersPayload }) {
  const [payload, setPayload] = useState(initial);
  const [loading, setLoading] = useState(false);

  const select = async (range: string, source: string) => {
    setLoading(true);
    try {
      const response = await fetch(`/api/daily-players?range=${range}&source=${source}`);
      if (response.ok) {
        setPayload(await response.json());
      }
    } finally {
      setLoading(false);
    }
  };

  const stacked = payload.keys !== undefined && payload.stacks !== undefined;
  const domain = eachUtcDay(parseUtcDay(payload.from), parseUtcDay(payload.to));

  return (
    <section className="flex flex-col gap-4">
      <header className="flex flex-row justify-between items-baseline">
        <h1 className="text-2xl font-bold clear-both">Daily players</h1>
        <span className="flex flex-row items-baseline gap-4">
          <Dropdown
            options={SOURCES}
            current={payload.source}
            onSelect={(source) => select(payload.range, source)}
          />
          <Dropdown
            options={PRESETS}
            current={payload.range}
            onSelect={(range) => select(range, payload.source)}
          />
        </span>
      </header>

      <div className={`transition-opacity ${loading ? 'opacity-50' : ''}`}>
        {stacked ? (
          <StackedBarChart
            keys={payload.keys!}
            points={(payload.stacks ?? []).map(({ day, values }) => ({
              date: parseUtcDay(day),
              values,
            }))}
            emptyLabel="No DDNet history yet — the import is still running"
          />
        ) : payload.source === 'teerank' ? (
          <BarChart
            points={fillSeries(
              domain,
              (payload.days ?? []).map(({ day, players }) => ({ at: parseUtcDay(day), value: players }))
            )}
            emptyLabel="No history yet — the first rollup lands tomorrow"
          />
        ) : (
          <BarChart
            points={(payload.days ?? []).map(({ day, players }) => ({
              date: parseUtcDay(day),
              value: players,
            }))}
            emptyLabel="No DDNet history yet — the import is still running"
          />
        )}
      </div>

      {payload.source !== 'teerank' && (
        <p className="text-sm text-[#999]">
          Average concurrent players on official DDNet servers since Dec 2014 — data from{' '}
          <a href="https://ddnet.org" className="hover:underline" target="_blank" rel="noopener">
            DDNet.org
          </a>
        </p>
      )}
    </section>
  );
}
