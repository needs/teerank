'use client';

import { useEffect, useRef, useState } from 'react';
import { format } from 'date-fns';
import { DAY_MS, addUtcDays, formatUtcDay, parseUtcDay } from '@teerank/teerank/date';

export type ActivityDay = {
  day: string; // YYYY-MM-DD
  value: number;
  tooltip: string;
};

export type ActivityPayload = {
  range: string;
  from: string;
  to: string;
  days: ActivityDay[];
};

const EMPTY_COLOR = '#00000010';
const LEVEL_COLORS = ['#e6dcae', '#d2ba6a', '#b3922e', '#997700'];

const TOOLTIP_DELAY_MS = 300;

function quantile(sorted: number[], q: number) {
  return sorted[Math.min(sorted.length - 1, Math.floor(sorted.length * q))];
}

function cellColor(value: number | undefined, thresholds: number[]) {
  if (value === undefined || value <= 0) {
    return EMPTY_COLOR;
  }

  for (const [level, threshold] of thresholds.entries()) {
    if (value <= threshold) {
      return LEVEL_COLORS[level];
    }
  }

  return LEVEL_COLORS[LEVEL_COLORS.length - 1];
}

type Tooltip = {
  x: number;
  y: number;
  text: string;
};

export function ActivityCalendar({
  payload,
  showLabels,
}: {
  payload: ActivityPayload;
  showLabels: boolean;
}) {
  const containerRef = useRef<HTMLDivElement>(null);
  const [containerWidth, setContainerWidth] = useState(0);
  const [tooltip, setTooltip] = useState<Tooltip | null>(null);
  const timerRef = useRef<ReturnType<typeof setTimeout>>();

  useEffect(() => {
    const container = containerRef.current;

    if (container === null) {
      return;
    }

    const observer = new ResizeObserver(() => setContainerWidth(container.clientWidth));
    observer.observe(container);
    setContainerWidth(container.clientWidth);

    return () => observer.disconnect();
  }, []);

  useEffect(() => () => clearTimeout(timerRef.current), []);

  const showTooltip = (event: React.MouseEvent<SVGRectElement>, text: string) => {
    const bounds = event.currentTarget.getBoundingClientRect();
    clearTimeout(timerRef.current);
    timerRef.current = setTimeout(() => {
      setTooltip({ x: bounds.left + bounds.width / 2, y: bounds.top, text });
    }, TOOLTIP_DELAY_MS);
  };

  const hideTooltip = () => {
    clearTimeout(timerRef.current);
    setTooltip(null);
  };

  const from = parseUtcDay(payload.from);
  const to = parseUtcDay(payload.to);
  const days = new Map(payload.days.map((day) => [day.day, day]));

  const positives = payload.days
    .map(({ value }) => value)
    .filter((value) => value > 0)
    .sort((a, b) => a - b);
  const thresholds = [quantile(positives, 0.25), quantile(positives, 0.5), quantile(positives, 0.75)];

  // Columns start on the Sunday on or before the first day.
  const startSunday = addUtcDays(from, -from.getUTCDay());
  const weeks = Math.floor((to.getTime() - startSunday.getTime()) / DAY_MS / 7) + 1;

  const pitch = containerWidth === 0 ? 14 : Math.min(22, Math.max(12, containerWidth / weeks));
  const cell = pitch * 0.78;
  const width = weeks * pitch - (pitch - cell);
  const height = 7 * pitch - (pitch - cell);

  const cells: React.ReactNode[] = [];
  const monthLabels: { week: number; label: string }[] = [];
  let previousMonth = -1;
  let lastLabeledColumn = -3;

  for (let week = 0; week < weeks; week++) {
    let columnMonth: number | null = null;

    for (let weekday = 0; weekday < 7; weekday++) {
      const date = addUtcDays(startSunday, week * 7 + weekday);

      if (date < from || date > to) {
        continue;
      }

      columnMonth ??= date.getUTCMonth();

      const iso = formatUtcDay(date);
      const day = days.get(iso);
      const text = day?.tooltip ?? `No activity on ${format(date, 'MMM d, yyyy')}`;

      cells.push(
        <rect
          key={iso}
          x={week * pitch}
          y={weekday * pitch}
          width={cell}
          height={cell}
          rx="2.5"
          fill={cellColor(day?.value, thresholds)}
          strokeWidth="1"
          className="stroke-transparent hover:stroke-[#00000059]"
          onMouseEnter={(event) => showTooltip(event, text)}
          onMouseLeave={hideTooltip}
        />
      );
    }

    // Label columns where a new month starts, keeping labels apart.
    if (columnMonth !== null && columnMonth !== previousMonth) {
      if (week - lastLabeledColumn >= 3 && week <= weeks - 3) {
        lastLabeledColumn = week;
        monthLabels.push({
          week,
          label: format(new Date(Date.UTC(2000, columnMonth, 1)), 'MMM'),
        });
      }
      previousMonth = columnMonth;
    }
  }

  return (
    <div ref={containerRef} className="relative w-full">
      <svg width={width} height={height} className="block max-w-full">
        {cells}
      </svg>

      <div
        className={`pointer-events-none absolute bottom-full left-0 mb-2 h-4 w-full transition-opacity ${
          showLabels ? 'opacity-100' : 'opacity-0'
        }`}
      >
        {monthLabels.map(({ week, label }) => (
          <span
            key={week}
            className="absolute bottom-0 text-[11px] leading-none text-[#999]"
            style={{ left: week * pitch }}
          >
            {label}
          </span>
        ))}
      </div>

      {tooltip !== null && (
        <div
          className="pointer-events-none fixed z-50 -translate-x-1/2 -translate-y-full whitespace-nowrap rounded-md bg-[#444] px-2 py-1 text-xs text-white shadow-md"
          style={{ left: tooltip.x, top: tooltip.y - 6 }}
        >
          {tooltip.text}
        </div>
      )}
    </div>
  );
}
