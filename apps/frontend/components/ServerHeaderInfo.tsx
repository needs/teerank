'use client';

import Link from 'next/link';
import { encodeString } from '../utils/encoding';
import { useSnapshot } from './SnapshotContext';

export function ServerHeaderInfo({
  name,
  gameTypeName,
  mapName,
  numClients,
  maxClients,
  playTime,
}: {
  name: string;
  gameTypeName: string;
  mapName: string;
  numClients: number;
  maxClients: number;
  playTime: string;
}) {
  const { selected, snapshot } = useSnapshot();
  const rewound = selected === null ? null : snapshot;

  const displayed =
    rewound === null
      ? { name, gameTypeName, mapName, numClients, maxClients }
      : {
          name: rewound.name,
          gameTypeName: rewound.map.gameTypeName,
          mapName: rewound.map.name,
          numClients: rewound.numClients,
          maxClients: rewound.maxClients,
        };

  return (
    <section className="flex flex-col gap-2">
      <h1 className="text-2xl font-bold">{displayed.name}</h1>
      <div className="flex flex-row divide-x">
        <span className="pr-4">
          <Link
            className="hover:underline"
            href={{
              pathname: `/gametype/${encodeString(displayed.gameTypeName)}`,
            }}
          >
            {displayed.gameTypeName}
          </Link>
        </span>
        <span className="px-4">
          <Link
            className="hover:underline"
            href={{
              pathname: `/gametype/${encodeString(
                displayed.gameTypeName
              )}/map/${encodeString(displayed.mapName)}`,
            }}
          >
            {displayed.mapName}
          </Link>
        </span>
        <span className="px-4">{`${displayed.numClients} / ${displayed.maxClients} clients`}</span>
        <span className="px-4">Playtime: {playTime}</span>
      </div>
    </section>
  );
}
