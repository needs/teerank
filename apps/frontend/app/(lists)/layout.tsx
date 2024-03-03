import { getGlobalCounts } from '../../utils/globalCounts';
import prisma from '../../utils/prisma';
import { LayoutTabs } from './LayoutTabs';

export default async function Index({
  children,
}: {
  children: React.ReactNode;
}) {
  const {
    playerCount,
    clanCount,
    gameServerCount,
  } = await getGlobalCounts();

  return (
    <div className="flex flex-col gap-4 py-8">
      <LayoutTabs
        playerCount={playerCount}
        clanCount={clanCount}
        serverCount={gameServerCount}
      />

      {children}
    </div>
  );
}
