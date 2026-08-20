'use client';

import { useState } from 'react';
import { DAY_MS, formatUtcDay, parseUtcDay, utcYesterday } from '@teerank/teerank/date';
import { ActivityCalendar, ActivityPayload } from './ActivityCalendar';

const ARROW_CLASSES =
  'absolute inset-y-0 z-20 flex w-8 items-center justify-center text-lg ' +
  'leading-none text-[#970] transition-opacity xl:w-20';

export function ActivityCalendarSection({
  apiPath,
  initial,
}: {
  apiPath: string;
  initial: ActivityPayload;
}) {
  const [payload, setPayload] = useState(initial);
  const [loading, setLoading] = useState(false);
  const [active, setActive] = useState(false);

  const atLatest = payload.to >= formatUtcDay(utcYesterday());
  const chromeClasses = active ? 'visible opacity-100' : 'invisible opacity-0';

  const shiftWindow = async (direction: 1 | -1) => {
    const from = parseUtcDay(payload.from);
    const to = parseUtcDay(payload.to);
    const spanMs = to.getTime() - from.getTime() + DAY_MS;

    let newFrom = new Date(from.getTime() + direction * spanMs);
    let newTo = new Date(to.getTime() + direction * spanMs);

    const yesterday = utcYesterday();
    if (newTo > yesterday) {
      newTo = yesterday;
      newFrom = new Date(newTo.getTime() - spanMs + DAY_MS);
    }

    setLoading(true);
    try {
      const response = await fetch(
        `${apiPath}?from=${formatUtcDay(newFrom)}&to=${formatUtcDay(newTo)}`
      );
      if (response.ok) {
        setPayload(await response.json());
      }
    } finally {
      setLoading(false);
    }
  };

  return (
    <div
      className={`relative -mx-8 px-8 xl:-mx-20 xl:px-20 ${active ? 'z-20' : 'z-0'}`}
      onMouseLeave={() => setActive(false)}
    >
      <button
        onClick={() => shiftWindow(-1)}
        aria-label="Earlier"
        className={`${ARROW_CLASSES} ${chromeClasses} left-0 hover:bg-gradient-to-r hover:from-[#f4efdc] hover:to-transparent`}
      >
        ‹
      </button>
      {!atLatest && (
        <button
          onClick={() => shiftWindow(1)}
          aria-label="Later"
          className={`${ARROW_CLASSES} ${chromeClasses} right-0 hover:bg-gradient-to-l hover:from-[#f4efdc] hover:to-transparent`}
        >
          ›
        </button>
      )}

      <div
        className={`rounded-md transition-colors ${active ? 'bg-white' : ''}`}
        onMouseEnter={() => setActive(true)}
      >
        <div className={`transition-opacity ${loading ? 'opacity-50' : ''}`}>
          <ActivityCalendar payload={payload} showLabels={active} />
        </div>
      </div>
    </div>
  );
}
