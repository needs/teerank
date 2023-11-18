'use client';

import { Tab, Tabs } from '@teerank/frontend/components/Tabs';
import { usePathname } from 'next/navigation';

export function LayoutTabs() {
  const pathname = usePathname();

  return (
    <Tabs>
      <Tab
        label="Players"
        count={590000}
        isActive={pathname === '/'}
        href={{
          pathname: '/',
        }}
      />
      <Tab
        label="Clans"
        count={60000}
        isActive={pathname === '/clans'}
        href={{ pathname: '/clans' }}
      />
      <Tab
        label="Servers"
        count={1200}
        isActive={pathname === '/servers'}
        href={{ pathname: '/servers' }}
      />
    </Tabs>
  );
}
