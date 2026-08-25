'use client';

import {
  createContext,
  useContext,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react';
import { useDebounce } from '../utils/hooks';

export type TimelinePoint = {
  id: number;
  createdAt: string;
  numClients: number;
};

export type Snapshot = {
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

type SnapshotContextValue = {
  snapshots: TimelinePoint[];
  selectedIndex: number | null;
  setSelectedIndex: (index: number | null) => void;
  selected: TimelinePoint | null;
  snapshot: Snapshot | null;
  stale: boolean;
};

const SnapshotContext = createContext<SnapshotContextValue>({
  snapshots: [],
  selectedIndex: null,
  setSelectedIndex: () => undefined,
  selected: null,
  snapshot: null,
  stale: false,
});

export function useSnapshot() {
  return useContext(SnapshotContext);
}

export function SnapshotProvider({
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

  const value = useMemo(
    () => ({
      snapshots,
      selectedIndex,
      setSelectedIndex,
      selected,
      snapshot,
      stale: selected !== null && snapshot?.id !== selected.id,
    }),
    [snapshots, selectedIndex, selected, snapshot]
  );

  return (
    <SnapshotContext.Provider value={value}>
      {children}
    </SnapshotContext.Provider>
  );
}
