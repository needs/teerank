'use client';

import { useState } from 'react';
import { BarChart } from './Chart';
import { fillSeries } from '../utils/series';
import { eachUtcDay, parseUtcDay } from '@teerank/teerank/date';
import { DailyPlayersPayload } from '../utils/dailyPlayers';

const PRESETS: { key: string; label: string }[] = [
  { key: '30d', label: 'Last 30 days' },
  { key: '90d', label: 'Last 90 days' },
  { key: '1y', label: 'Last year' },
  { key: 'all', label: 'All time' },
];

export function DailyPlayersSection({ initial }: { initial: DailyPlayersPayload }) {
  const [payload, setPayload] = useState(initial);
  const [loading, setLoading] = useState(false);
  const [open, setOpen] = useState(false);

  const select = async (range: string) => {
    setOpen(false);
    setLoading(true);
    try {
      const response = await fetch(`/api/daily-players?range=${range}`);
      if (response.ok) {
        setPayload(await response.json());
      }
    } finally {
      setLoading(false);
    }
  };

  const domain = eachUtcDay(parseUtcDay(payload.from), parseUtcDay(payload.to));

  return (
    <section className="flex flex-col gap-4">
      <header className="flex flex-row justify-between items-baseline">
        <h1 className="text-2xl font-bold clear-both">Daily players</h1>
        <span className="relative">
          <button
            onClick={() => setOpen(!open)}
            className="text-md clear-both text-[#888] hover:underline"
          >
            {PRESETS.find(({ key }) => key === payload.range)?.label ?? payload.range} ▾
          </button>

          {open && (
            <>
              <span className="fixed inset-0 z-10" onClick={() => setOpen(false)} />
              <span className="absolute right-0 top-full z-20 mt-1 flex w-max flex-col rounded-md border bg-white p-1 shadow-md">
                {PRESETS.map((preset) => (
                  <button
                    key={preset.key}
                    onClick={() => select(preset.key)}
                    className={`rounded px-3 py-1 text-left text-sm hover:bg-[#f4efdc] ${
                      payload.range === preset.key ? 'font-bold text-[#970]' : 'text-[#666]'
                    }`}
                  >
                    {preset.label}
                  </button>
                ))}
              </span>
            </>
          )}
        </span>
      </header>

      <div className={`transition-opacity ${loading ? 'opacity-50' : ''}`}>
        <BarChart
          points={fillSeries(
            domain,
            payload.days.map(({ day, players }) => ({ at: parseUtcDay(day), value: players }))
          )}
          emptyLabel="No history yet — the first rollup lands tomorrow"
        />
      </div>
    </section>
  );
}
