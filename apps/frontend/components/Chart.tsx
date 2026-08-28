'use client';

import { useState } from 'react';
import { format } from 'date-fns';
import { DAY_MS, localizeUtcDay } from '@teerank/teerank/date';
import { formatFinishTime, formatInteger } from '../utils/format';

export type ChartPoint = {
  date: Date;
  value: number | null;
};

const WIDTH = 1000;
const HEIGHT = 260;
const MARGIN = { top: 12, right: 12, bottom: 32, left: 90 };
const INNER_WIDTH = WIDTH - MARGIN.left - MARGIN.right;
const INNER_HEIGHT = HEIGHT - MARGIN.top - MARGIN.bottom;

function axisDatePattern(dates: Date[]) {
  const span = (dates[dates.length - 1].getTime() - dates[0].getTime()) / DAY_MS;

  if (span > 366) {
    return 'MMM yyyy';
  }

  return dates[0].getUTCFullYear() === dates[dates.length - 1].getUTCFullYear()
    ? 'MMM d'
    : 'MMM d, yyyy';
}

function formatUtcDate(date: Date, pattern: string) {
  return format(localizeUtcDay(date), pattern);
}

function niceCeiling(value: number) {
  if (value <= 0) {
    return 1;
  }

  const magnitude = Math.pow(10, Math.floor(Math.log10(value)));

  for (const factor of [1, 2, 2.5, 5, 10]) {
    if (factor * magnitude >= value) {
      return factor * magnitude;
    }
  }

  return 10 * magnitude;
}

function slotX(index: number, count: number) {
  return MARGIN.left + ((index + 0.5) / count) * INNER_WIDTH;
}

function xTickIndices(count: number) {
  const tickCount = Math.min(5, count);
  const indices = new Set<number>();

  for (let tick = 0; tick < tickCount; tick++) {
    indices.add(Math.round((tick / Math.max(1, tickCount - 1)) * (count - 1)));
  }

  return [...indices];
}

function XAxis({
  dates,
  formatDate,
  fontSize,
}: {
  dates: Date[];
  formatDate: (date: Date) => string;
  fontSize: number;
}) {
  return (
    <>
      {xTickIndices(dates.length).map((index) => (
        <text
          key={index}
          x={slotX(index, dates.length)}
          y={HEIGHT - 10}
          textAnchor={index === 0 ? 'start' : index === dates.length - 1 ? 'end' : 'middle'}
          fontSize={fontSize}
          fill="#999"
        >
          {formatDate(dates[index])}
        </text>
      ))}
    </>
  );
}

function YAxis({
  ticks,
  scaleY,
  formatValue,
  fontSize,
}: {
  ticks: number[];
  scaleY: (value: number) => number;
  formatValue: (value: number) => string;
  fontSize: number;
}) {
  return (
    <>
      {ticks.map((tick) => (
        <g key={tick}>
          <line
            x1={MARGIN.left}
            x2={WIDTH - MARGIN.right}
            y1={scaleY(tick)}
            y2={scaleY(tick)}
            stroke="#00000012"
          />
          <text
            x={MARGIN.left - 8}
            y={scaleY(tick) + 5}
            textAnchor="end"
            fontSize={fontSize}
            fill="#999"
          >
            {formatValue(tick)}
          </text>
        </g>
      ))}
    </>
  );
}

function EmptyChart({ label }: { label: string }) {
  return (
    <div className="flex h-32 items-center justify-center border rounded-md bg-[#fafafa] text-sm text-[#999]">
      {label}
    </div>
  );
}

export function BarChart({
  points,
  formatDate,
  formatTooltipDate,
  formatValue = formatInteger,
  emptyLabel = 'No data yet',
  fontSize = 14,
}: {
  points: ChartPoint[];
  formatDate?: (date: Date) => string;
  formatTooltipDate?: (date: Date) => string;
  formatValue?: (value: number) => string;
  emptyLabel?: string;
  // Bump for charts rendered at half width, where the viewBox scales text down.
  fontSize?: number;
}) {
  const [tooltip, setTooltip] = useState<{ x: number; y: number; text: string } | null>(null);

  const showTooltip = (event: React.MouseEvent<SVGRectElement>, text: string) => {
    const bounds = event.currentTarget.getBoundingClientRect();
    setTooltip({ x: bounds.left + bounds.width / 2, y: bounds.top, text });
  };

  const hideTooltip = () => {
    setTooltip(null);
  };

  const values = points.flatMap((point) => (point.value === null ? [] : point.value));

  if (values.length === 0) {
    return <EmptyChart label={emptyLabel} />;
  }

  const dates = points.map((point) => point.date);
  const axisDate = formatDate ?? ((date: Date) => formatUtcDate(date, axisDatePattern(dates)));
  const tooltipDate =
    formatTooltipDate ?? ((date: Date) => formatUtcDate(date, 'MMM d, yyyy'));

  const max = niceCeiling(Math.max(...values));
  const scaleY = (value: number) => MARGIN.top + INNER_HEIGHT * (1 - value / max);
  const barWidth = (INNER_WIDTH / points.length) * 0.7;

  return (
    <div className="relative">
      <svg
        viewBox={`0 0 ${WIDTH} ${HEIGHT}`}
        preserveAspectRatio="xMidYMid meet"
        className="w-full"
        role="img"
      >
        <YAxis ticks={[0, max / 2, max]} scaleY={scaleY} formatValue={formatValue} fontSize={fontSize} />
        <XAxis dates={dates} formatDate={axisDate} fontSize={fontSize} />

        {points.map((point, index) =>
          point.value === null ? null : (
            <rect
              key={point.date.getTime()}
              x={slotX(index, points.length) - barWidth / 2}
              y={Math.min(scaleY(point.value), MARGIN.top + INNER_HEIGHT - 1.5)}
              width={barWidth}
              height={Math.max(1.5, INNER_HEIGHT * (point.value / max))}
              fill="#997700"
              fillOpacity="0.7"
              strokeWidth="1"
              className="stroke-transparent transition-[fill-opacity] hover:[fill-opacity:1] hover:stroke-[#00000059]"
              onMouseEnter={(event) =>
                showTooltip(event, `${formatValue(point.value as number)} on ${tooltipDate(point.date)}`)
              }
              onMouseLeave={hideTooltip}
            />
          )
        )}
      </svg>

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

export type StepPoint = {
  date: Date;
  value: number;
  label: string;
};

const STEP_HEIGHT = 150;
const STEP_INNER_HEIGHT = STEP_HEIGHT - MARGIN.top - MARGIN.bottom;

export function StepLineChart({
  points,
  endDate,
  formatValue = formatFinishTime,
  fontSize = 14,
}: {
  points: StepPoint[];
  endDate?: Date;
  formatValue?: (value: number) => string;
  fontSize?: number;
}) {
  const [tooltip, setTooltip] = useState<{ x: number; y: number; text: string } | null>(null);

  if (points.length < 2) {
    return null;
  }

  const showTooltip = (event: React.MouseEvent<SVGCircleElement>, text: string) => {
    const bounds = event.currentTarget.getBoundingClientRect();
    setTooltip({ x: bounds.left + bounds.width / 2, y: bounds.top, text });
  };

  const hideTooltip = () => {
    setTooltip(null);
  };

  const start = points[0].date.getTime();
  const end = Math.max(points[points.length - 1].date.getTime(), endDate?.getTime() ?? 0);
  const span = Math.max(1, end - start);
  const scaleX = (time: number) => MARGIN.left + ((time - start) / span) * INNER_WIDTH;

  const values = points.map((point) => point.value);
  let low = Math.min(...values);
  let high = Math.max(...values);

  if (low === high) {
    low -= 1;
    high += 1;
  }

  const pad = (high - low) * 0.08;
  const scaleY = (value: number) =>
    MARGIN.top +
    STEP_INNER_HEIGHT * (1 - (value - (low - pad)) / (high - low + 2 * pad));

  const path =
    points
      .map((point, index) => {
        const x = scaleX(point.date.getTime());
        const y = scaleY(point.value);
        return index === 0 ? `M ${x} ${y}` : `H ${x} V ${y}`;
      })
      .join(' ') + ` H ${WIDTH - MARGIN.right}`;

  const yearTicks: number[] = [];

  for (
    let year = new Date(start).getUTCFullYear() + 1;
    year <= new Date(end).getUTCFullYear();
    year++
  ) {
    yearTicks.push(Date.UTC(year, 0, 1));
  }

  const tickStep = Math.max(1, Math.ceil(yearTicks.length / 6));
  const xTicks =
    yearTicks.length > 0
      ? yearTicks
          .filter((_, index) => index % tickStep === 0)
          .map((time) => ({ time, label: format(new Date(time), 'yyyy') }))
      : [
          { time: start, label: format(new Date(start), 'MMM yyyy') },
          { time: end, label: format(new Date(end), 'MMM yyyy') },
        ];

  return (
    <div className="relative">
      <svg
        viewBox={`0 0 ${WIDTH} ${STEP_HEIGHT}`}
        preserveAspectRatio="xMidYMid meet"
        className="w-full"
        role="img"
      >
        <YAxis
          ticks={[low, (low + high) / 2, high]}
          scaleY={scaleY}
          formatValue={formatValue}
          fontSize={fontSize}
        />

        {xTicks.map((tick, index) => (
          <text
            key={tick.time}
            x={scaleX(tick.time)}
            y={STEP_HEIGHT - 10}
            textAnchor={
              index === 0 ? 'start' : index === xTicks.length - 1 ? 'end' : 'middle'
            }
            fontSize={fontSize}
            fill="#999"
          >
            {tick.label}
          </text>
        ))}

        <path d={path} fill="none" stroke="#997700" strokeWidth="2" />

        {points.map((point) => (
          <circle
            key={`${point.date.getTime()}-${point.value}`}
            cx={scaleX(point.date.getTime())}
            cy={scaleY(point.value)}
            r={5}
            fill="#997700"
            fillOpacity="0.85"
            className="transition-[fill-opacity] hover:[fill-opacity:1]"
            onMouseEnter={(event) => showTooltip(event, point.label)}
            onMouseLeave={hideTooltip}
          />
        ))}
      </svg>

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

const STACK_COLORS = [
  '#997700', '#5f7a8c', '#a05a2c', '#4a7c59',
  '#8c5f7a', '#c2a14d', '#6b7a4a', '#999999',
];

export function StackedBarChart({
  keys,
  points,
  formatDate,
  formatTooltipDate,
  formatValue = formatInteger,
  emptyLabel = 'No data yet',
  fontSize = 14,
}: {
  keys: string[];
  points: { date: Date; values: number[] }[];
  formatDate?: (date: Date) => string;
  formatTooltipDate?: (date: Date) => string;
  formatValue?: (value: number) => string;
  emptyLabel?: string;
  fontSize?: number;
}) {
  const [tooltip, setTooltip] = useState<{ x: number; y: number; lines: string[] } | null>(null);

  if (points.length === 0 || keys.length === 0) {
    return <EmptyChart label={emptyLabel} />;
  }

  const showTooltip = (event: React.MouseEvent<SVGGElement>, lines: string[]) => {
    const bounds = event.currentTarget.getBoundingClientRect();
    setTooltip({ x: bounds.left + bounds.width / 2, y: bounds.top, lines });
  };

  const hideTooltip = () => {
    setTooltip(null);
  };

  const dates = points.map((point) => point.date);
  const axisDate = formatDate ?? ((date: Date) => formatUtcDate(date, axisDatePattern(dates)));
  const tooltipDate =
    formatTooltipDate ?? ((date: Date) => formatUtcDate(date, 'MMM d, yyyy'));

  const totals = points.map((point) => point.values.reduce((sum, value) => sum + value, 0));
  const max = niceCeiling(Math.max(...totals, 1));
  const scaleY = (value: number) => MARGIN.top + INNER_HEIGHT * (1 - value / max);
  const barWidth = (INNER_WIDTH / points.length) * 0.7;

  return (
    <div className="relative">
      <svg
        viewBox={`0 0 ${WIDTH} ${HEIGHT}`}
        preserveAspectRatio="xMidYMid meet"
        className="w-full"
        role="img"
      >
        <YAxis ticks={[0, max / 2, max]} scaleY={scaleY} formatValue={formatValue} fontSize={fontSize} />
        <XAxis dates={dates} formatDate={axisDate} fontSize={fontSize} />

        {points.map((point, index) => {
          const x = slotX(index, points.length) - barWidth / 2;
          const lines = [
            tooltipDate(point.date),
            ...keys.flatMap((key, keyIndex) =>
              point.values[keyIndex] > 0 ? `${key}: ${formatValue(point.values[keyIndex])}` : []
            ),
          ];

          let stacked = 0;

          return (
            <g
              key={point.date.getTime()}
              className="transition-opacity hover:opacity-80"
              onMouseEnter={(event) => showTooltip(event, lines)}
              onMouseLeave={hideTooltip}
            >
              {keys.map((key, keyIndex) => {
                const value = point.values[keyIndex];
                if (value <= 0) {
                  return null;
                }
                const y = scaleY(stacked + value);
                const height = INNER_HEIGHT * (value / max);
                stacked += value;
                return (
                  <rect
                    key={key}
                    x={x}
                    y={y}
                    width={barWidth}
                    height={Math.max(0.5, height)}
                    fill={STACK_COLORS[keyIndex % STACK_COLORS.length]}
                    fillOpacity="0.75"
                  />
                );
              })}
            </g>
          );
        })}
      </svg>

      <div className="flex flex-row flex-wrap gap-x-4 gap-y-1 px-2 text-sm text-[#888]">
        {keys.map((key, index) => (
          <span key={key} className="flex flex-row items-center gap-1.5">
            <span
              className="inline-block h-2.5 w-2.5 rounded-full"
              style={{ background: STACK_COLORS[index % STACK_COLORS.length] }}
            />
            {key}
          </span>
        ))}
      </div>

      {tooltip !== null && (
        <div
          className="pointer-events-none fixed z-50 -translate-x-1/2 -translate-y-full whitespace-nowrap rounded-md bg-[#444] px-2 py-1 text-xs text-white shadow-md"
          style={{ left: tooltip.x, top: tooltip.y - 6 }}
        >
          {tooltip.lines.map((line) => (
            <div key={line}>{line}</div>
          ))}
        </div>
      )}
    </div>
  );
}
