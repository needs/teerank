import { getGlobalCounts } from '@teerank/teerank';
import redis from '../../utils/redis';
import { LayoutTabs } from './LayoutTabs';

export default async function Index({
  children,
}: {
  children: React.ReactNode;
}) {
  const globalCounts = await getGlobalCounts(redis);

  return (
    <div className="flex flex-col gap-4 py-8">
      <LayoutTabs
        playerCount={globalCounts.players}
        clanCount={globalCounts.clans}
        serverCount={globalCounts.gameServers}
      />

      {children}
    </div>
  );
}
