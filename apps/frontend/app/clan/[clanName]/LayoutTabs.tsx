'use client';

import { Tab, Tabs } from '../../../components/Tabs';
import { usePathname } from 'next/navigation';

export function LayoutTabs({
  clanName, playerCount, gameTypesCount, mapsCount,
}: {
  clanName: string;
  playerCount: number;
  gameTypesCount: number;
  mapsCount: number;
}) {
  const urlPathname = `/clan/${encodeURIComponent(clanName)}`;
  const pathname = usePathname();

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
        label="Game Types"
        count={gameTypesCount}
        isActive={pathname === `${urlPathname}/gametypes`}
        href={{ pathname: `${urlPathname}/gametypes` }}
      />
      <Tab
        label="Maps"
        count={mapsCount}
        isActive={pathname === `${urlPathname}/maps`}
        href={{ pathname: `${urlPathname}/maps` }}
      />
    </Tabs>
  );
}
