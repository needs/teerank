'use client';

import { Tab, Tabs } from '../../components/Tabs';
import { usePathname } from 'next/navigation';

export function LayoutTabs({
  playerCount, clanCount, serverCount,
}: {
  playerCount: number;
  clanCount: number;
  serverCount: number;
}) {
  const pathname = usePathname();

  return (
    <Tabs>
      <Tab
        label="Players"
        count={playerCount}
        isActive={pathname === '/all'}
        href={{
          pathname: '/all',
        }}
      />
      <Tab
        label="Clans"
        count={clanCount}
        isActive={pathname === '/all/clans'}
        href={{ pathname: '/all/clans' }}
      />
      <Tab
        label="Servers"
        count={serverCount}
        isActive={pathname === '/all/servers'}
        href={{ pathname: '/all/servers' }}
      />
    </Tabs>
  );
}
