'use client';

import { Tab, Tabs } from '@teerank/frontend/components/Tabs';
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
        isActive={pathname === '/'}
        href={{
          pathname: '/',
        }}
      />
      <Tab
        label="Clans"
        count={clanCount}
        isActive={pathname === '/clans'}
        href={{ pathname: '/clans' }}
      />
      <Tab
        label="Servers"
        count={serverCount}
        isActive={pathname === '/servers'}
        href={{ pathname: '/servers' }}
      />
    </Tabs>
  );
}
