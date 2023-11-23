'use client';

import { Tab, Tabs } from '@teerank/frontend/components/Tabs';
import { usePathname } from 'next/navigation';

export function LayoutTabs({
  gameTypeName,
  mapName,
  playerCount,
  clanCount,
  serverCount,
}: {
  gameTypeName: string;
  mapName?: string;
  playerCount: number;
  clanCount: number;
  serverCount: number;
}) {
  const pathname = usePathname();

  const urlPathname = `/gametype/${encodeURIComponent(gameTypeName)}${
    mapName === undefined ? '' : `/map/${encodeURIComponent(mapName)}`
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
    </Tabs>
  );
}
