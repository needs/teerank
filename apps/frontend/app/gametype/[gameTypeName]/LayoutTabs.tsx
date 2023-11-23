'use client';

import { Tab, Tabs } from '@teerank/frontend/components/Tabs';
import { usePathname } from 'next/navigation';

export function LayoutTabs({
  gameTypeName,
  mapName,
  playerCount,
  serverCount,
}: {
  gameTypeName: string;
  mapName?: string;
  playerCount: number;
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
        label="Maps"
        count={60000}
        isActive={pathname === `${urlPathname}/maps`}
        href={{ pathname: `${urlPathname}/maps` }}
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
