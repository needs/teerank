'use client';

import { useState } from 'react';
import { format } from 'date-fns';
import { formatInteger } from '../utils/format';

export type ChartPoint = {
  date: Date;
  value: number | null;
};

const WIDTH = 1000;
const HEIGHT = 260;
const MARGIN = { top: 12, right: 12, bottom: 32, left: 90 };
const INNER_WIDTH = WIDTH - MARGIN.left - MARGIN.right;
const INNER_HEIGHT = HEIGHT - MARGIN.top - MARGIN.bottom;

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
  formatDate = (date) => format(date, 'MMM d'),
  formatTooltipDate = formatDate,
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
        <XAxis dates={points.map((point) => point.date)} formatDate={formatDate} fontSize={fontSize} />

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
                showTooltip(event, `${formatValue(point.value as number)} on ${formatTooltipDate(point.date)}`)
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
