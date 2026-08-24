'use client';

import { Tab, Tabs } from '../../../components/Tabs';
import { usePathname } from 'next/navigation';
import { encodeString } from '../../../utils/encoding';

export function LayoutTabs({
  playerName,
  clanCount,
  teammateCount,
}: {
  playerName: string;
  clanCount: number;
  teammateCount: number;
}) {
  const urlPathname = `/player/${encodeString(playerName)}`;
  const pathname = usePathname();

  return (
    <Tabs>
      <Tab
        label="Playtime"
        isActive={pathname === `${urlPathname}`}
        href={{ pathname: `${urlPathname}` }}
      />
      <Tab
        label="Activity"
        isActive={pathname === `${urlPathname}/activity`}
        href={{ pathname: `${urlPathname}/activity` }}
      />
      <Tab
        label="Clans"
        count={clanCount}
        isActive={pathname === `${urlPathname}/clans`}
        href={{ pathname: `${urlPathname}/clans` }}
      />
      <Tab
        label="Teammates"
        count={teammateCount}
        isActive={pathname === `${urlPathname}/teammates`}
        href={{ pathname: `${urlPathname}/teammates` }}
      />
    </Tabs>
  );
}
