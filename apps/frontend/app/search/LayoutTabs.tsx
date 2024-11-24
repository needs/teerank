import React from 'react';
import { Tabs, Tab } from '../../components/Tabs';

export function LayoutTabs({
  children,
  selectedTab,
  query,
  playerCount,
  clanCount,
  gameServerCount,
}: {
  children: React.ReactNode;
  selectedTab: 'players' | 'clans' | 'servers';
  query: string;
  playerCount: number;
  clanCount: number;
  gameServerCount: number;
}) {
  return (
    <div className="flex flex-col gap-4 py-8">
      <Tabs>
        <Tab
          label="Players"
          count={playerCount}
          isActive={selectedTab === 'players'}
          href={{
            pathname: '/search',
            query: { query },
          }}
        />
        <Tab
          label="Clans"
          count={clanCount}
          isActive={selectedTab === 'clans'}
          href={{ pathname: '/search/clans', query: { query } }}
        />
        <Tab
          label="Servers"
          count={gameServerCount}
          isActive={selectedTab === 'servers'}
          href={{ pathname: '/search/servers', query: { query } }}
        />
      </Tabs>
      {children}
    </div>
  );
}
