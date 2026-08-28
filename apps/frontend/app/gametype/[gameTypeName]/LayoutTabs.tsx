'use client';

import { Tab, Tabs } from '../../../components/Tabs';
import { usePathname } from 'next/navigation';
import { encodeString } from '../../../utils/encoding';

export function LayoutTabs({
  gameTypeName,
  mapName,
  playerCount,
  clanCount,
  serverCount,
  showRecords,
}: {
  gameTypeName: string;
  mapName?: string;
  playerCount: number;
  clanCount: number;
  serverCount: number;
  showRecords?: boolean;
}) {
  const pathname = usePathname();

  const urlPathname = `/gametype/${encodeString(gameTypeName)}${
    mapName === undefined ? '' : `/map/${encodeString(mapName)}`
  }`;

  return (
    <Tabs>
      <Tab
        label="Players"
        count={playerCount}
        isActive={pathname === urlPathname}
        href={{
          pathname: urlPathname,
        }}
      />
      <Tab
        label="Clans"
        count={clanCount}
        isActive={pathname === `${urlPathname}/clans`}
        href={{ pathname: `${urlPathname}/clans` }}
      />
      <Tab
        label="Servers"
        count={serverCount}
        isActive={pathname === `${urlPathname}/servers`}
        href={{ pathname: `${urlPathname}/servers` }}
      />
      {showRecords && (
        <Tab
          label="Records"
          isActive={pathname === `${urlPathname}/records`}
          href={{ pathname: `${urlPathname}/records` }}
        />
      )}
    </Tabs>
  );
}
