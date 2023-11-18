'use client';

import { Tab, Tabs } from '@teerank/frontend/components/Tabs';
import { usePathname } from 'next/navigation';

export function LayoutTabs({
  playersCount, clansCount, serversCount,
}: {
  playersCount: number;
  clansCount: number;
  serversCount: number;
}) {
  const pathname = usePathname();

  return (
    <Tabs>
      <Tab
        label="Players"
        count={playersCount}
        isActive={pathname === '/'}
        href={{
          pathname: '/',
        }}
      />
      <Tab
        label="Clans"
        count={clansCount}
        isActive={pathname === '/clans'}
        href={{ pathname: '/clans' }}
      />
      <Tab
        label="Servers"
        count={serversCount}
        isActive={pathname === '/servers'}
        href={{ pathname: '/servers' }}
      />
    </Tabs>
  );
}
