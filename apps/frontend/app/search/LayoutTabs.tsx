import React from 'react';
import { Tabs, Tab } from '../../components/Tabs';
import prisma from '../../utils/prisma';

export async function LayoutTabs({
  children,
  query,
  selectedTab,
}: {
  children: (counts: { playerCount: number, clanCount: number, gameServerCount: number }) => React.ReactNode;
  query: string;
  selectedTab: 'players' | 'clans' | 'servers';
}) {
  const [playerCount, clanCount, gameServerCount] = await Promise.all([
    prisma.player.count({
      where: {
        name: {
          contains: query,
          mode: 'insensitive',
        },
      },
    }),
    prisma.clan.count({
      where: {
        name: {
          contains: query,
          mode: 'insensitive',
        },
      },
    }),
    prisma.gameServer.count({
      where: {
        lastSnapshot: {
          name: {
            contains: query,
            mode: 'insensitive',
          },
        },
      },
    }),
  ]);

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
      {children({ playerCount, clanCount, gameServerCount })}
    </div>
  );
}
