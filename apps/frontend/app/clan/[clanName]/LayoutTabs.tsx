'use client';

import { Tab, Tabs } from '../../../components/Tabs';
import { usePathname } from 'next/navigation';
import { encodeString } from '../../../utils/encoding';

export function LayoutTabs({
  clanName, playerCount, gameTypeCount, mapCount,
}: {
  clanName: string;
  playerCount: number;
  gameTypeCount: number;
  mapCount: number;
}) {
  const urlPathname = `/clan/${encodeString(clanName)}`;
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
        count={gameTypeCount}
        isActive={pathname === `${urlPathname}/gametypes`}
        href={{ pathname: `${urlPathname}/gametypes` }}
      />
      <Tab
        label="Maps"
        count={mapCount}
        isActive={pathname === `${urlPathname}/maps`}
        href={{ pathname: `${urlPathname}/maps` }}
      />
    </Tabs>
  );
}
