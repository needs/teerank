'use client';

import { Tab, Tabs } from '../../../components/Tabs';
import { usePathname } from 'next/navigation';

export function LayoutTabs({
  playerName, mapCount, gameTypeCount, clanCount,
}: {
  playerName: string;
  mapCount: number;
  gameTypeCount: number;
  clanCount: number;
}) {
  const urlPathname = `/player/${encodeURIComponent(playerName)}`;
  const pathname = usePathname();

  return (
    <Tabs>
      <Tab
        label="Game Types"
        count={gameTypeCount}
        isActive={pathname === `${urlPathname}`}
        href={{ pathname: `${urlPathname}` }}
      />
      <Tab
        label="Maps"
        count={mapCount}
        isActive={pathname === `${urlPathname}/maps`}
        href={{
          pathname: `${urlPathname}/maps`,
        }}
      />
      <Tab
        label="Clans"
        count={clanCount}
        isActive={pathname === `${urlPathname}/clans`}
        href={{ pathname: `${urlPathname}/clans` }}
      />
    </Tabs>
  );
}
