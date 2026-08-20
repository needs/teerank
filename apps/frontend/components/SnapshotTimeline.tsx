'use client';

import { Fragment, useEffect, useRef, useState } from 'react';
import { format } from 'date-fns';
import { List, ListCell } from './List';
import { encodeString } from '../utils/encoding';
import { useDebounce } from '../utils/hooks';

export type TimelinePoint = {
  id: number;
  createdAt: string;
  numClients: number;
};

type Snapshot = {
  id: number;
  createdAt: string;
  name: string;
  numClients: number;
  maxClients: number;
  map: {
    name: string;
    gameTypeName: string;
  };
  clients: {
    playerName: string;
    clanName: string | null;
    score: number;
  }[];
};

function sparklinePath(snapshots: TimelinePoint[]) {
  const maxClients = Math.max(1, ...snapshots.map(({ numClients }) => numClients));

  const points = snapshots.map(({ numClients }, index) => {
    const x =
      snapshots.length === 1
        ? 1000
        : (index / (snapshots.length - 1)) * 1000;
    const y = 40 - (numClients / maxClients) * 36;
    return `L${x},${y}`;
  });

  return `M0,40 ${points.join(' ')} L1000,40 Z`;
}

function SnapshotList({ snapshot }: { snapshot: Snapshot }) {
  return (
    <List
      columns={[
        {
          title: '',
          expand: false,
        },
        {
          title: 'Name',
          expand: true,
        },
        {
          title: 'Clan',
          expand: true,
        },
        {
          title: 'Score',
          expand: false,
        },
      ]}
    >
      {snapshot.clients.map((client, index) => (
        <Fragment key={`${snapshot.id}-${client.playerName}`}>
          <ListCell alignRight label={`${index + 1}`} />
          <ListCell
            label={client.playerName}
            href={{
              pathname: `/player/${encodeString(client.playerName)}`,
            }}
          />
          <ListCell
            label={client.clanName ?? ''}
            href={
              client.clanName === null
                ? undefined
                : {
                    pathname: `/clan/${encodeString(client.clanName)}`,
                  }
            }
          />
          <ListCell alignRight label={client.score.toString()} />
        </Fragment>
      ))}
    </List>
  );
}

export function SnapshotTimeline({
  snapshots,
  apiPath,
  children,
}: {
  snapshots: TimelinePoint[];
  apiPath: string;
  children: React.ReactNode;
}) {
  const [selectedIndex, setSelectedIndex] = useState<number | null>(null);
  const [lastLoaded, setLastLoaded] = useState<Snapshot | null>(null);
  const cacheRef = useRef<Map<number, Snapshot>>();

  if (cacheRef.current === undefined) {
    cacheRef.current = new Map();
  }

  const cache = cacheRef.current;
  const selected = selectedIndex === null ? null : snapshots[selectedIndex];
  const debouncedSelected = useDebounce(selected, 150);
  const snapshot =
    selected === null ? null : cache.get(selected.id) ?? lastLoaded;

  useEffect(() => {
    if (debouncedSelected === null || cache.has(debouncedSelected.id)) {
      return;
    }

    const controller = new AbortController();

    (async () => {
      try {
        const response = await fetch(`${apiPath}/${debouncedSelected.id}`, {
          signal: controller.signal,
        });

        if (!response.ok) {
          return;
        }

        const data: Snapshot = await response.json();
        cache.set(data.id, data);
        setLastLoaded(data);
      } catch {
        return;
      }
    })();

    return () => {
      controller.abort();
    };
  }, [debouncedSelected, apiPath, cache]);

  if (snapshots.length === 0) {
    return <>{children}</>;
  }

  const lastIndex = snapshots.length - 1;
  const cursorFraction =
    selectedIndex === null || lastIndex === 0 ? 1 : selectedIndex / lastIndex;
  const stale = selected !== null && snapshot?.id !== selected.id;

  return (
    <>
      <section className="flex flex-col gap-2 px-4 lg:px-8 xl:px-16">
        <div className="flex flex-row items-baseline justify-between text-sm">
          {selected === null ? (
            <span>
              <span className="font-bold text-[#970]">Live</span>
              <span className="text-[#666]"> — drag the bar to rewind</span>
            </span>
          ) : (
            <span>
              <span className="font-bold text-[#970]">
                {format(new Date(selected.createdAt), 'MMM d, HH:mm')}
              </span>
              <span className="text-[#666]">
                {' '}
                — {selected.numClients} clients
                {!stale && snapshot !== null && ` on ${snapshot.map.name}`}
              </span>
            </span>
          )}

          {selected !== null && (
            <button
              onClick={() => setSelectedIndex(null)}
              className="text-[#970] hover:underline"
            >
              Back to live
            </button>
          )}
        </div>

        <div className="relative h-10 border rounded-md overflow-hidden bg-[#fafafa]">
          <svg
            className="absolute inset-0 w-full h-full"
            viewBox="0 0 1000 40"
            preserveAspectRatio="none"
          >
            <path
              d={sparklinePath(snapshots)}
              fill="#997700"
              fillOpacity="0.15"
              stroke="#997700"
              strokeOpacity="0.4"
              strokeWidth="1"
              vectorEffect="non-scaling-stroke"
            />
          </svg>

          <div
            className="absolute top-0 h-full w-0.5 -ml-px bg-[#970] pointer-events-none"
            style={{
              left: `${cursorFraction * 100}%`,
            }}
          />

          <input
            type="range"
            min={0}
            max={lastIndex}
            step={1}
            value={selectedIndex ?? lastIndex}
            onChange={(event) => setSelectedIndex(Number(event.target.value))}
            aria-label="Rewind to a past snapshot"
            className="absolute inset-0 w-full h-full opacity-0 cursor-ew-resize"
          />
        </div>

        <div className="flex flex-row justify-between text-xs text-[#666]">
          <span suppressHydrationWarning>
            {format(new Date(snapshots[0].createdAt), 'MMM d, HH:mm')}
          </span>
          <span>now</span>
        </div>
      </section>

      {selected === null ? (
        children
      ) : snapshot === null ? (
        <p className="text-center text-[#666]">Loading snapshot…</p>
      ) : (
        <div className={stale ? 'opacity-50 transition-opacity' : ''}>
          <SnapshotList snapshot={snapshot} />
        </div>
      )}
    </>
  );
}
