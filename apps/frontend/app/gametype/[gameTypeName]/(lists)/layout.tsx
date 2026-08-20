import { notFound } from 'next/navigation';
import prisma from '../../../../utils/prisma';
import { LayoutTabs } from '../LayoutTabs';
import { paramsSchema } from '../schema';
import { encodeString } from '../../../../utils/encoding';
import { ActivityCalendarSection } from '../../../../components/ActivityCalendarSection';
import { getGameTypeActivity } from '../../../../utils/activity';

export default async function Index({
  children,
  params,
}: {
  children: React.ReactNode;
  params: { gameType: string };
}) {
  const { gameTypeName } = paramsSchema.parse(params);

  const gameType = await prisma.gameType.findUnique({
    where: {
      name: gameTypeName,
    },
  });

  if (gameType === null) {
    notFound();
  }

  const activity = await getGameTypeActivity(gameType.id, { range: '1y' });

  return (
    <div className="flex flex-col gap-4 py-8">
      <header className="px-8 xl:px-20">
        <div className="relative">
          <ActivityCalendarSection
            apiPath={`/api/gametype/${encodeString(gameTypeName)}/activity`}
            initial={activity}
          />
          <div className="absolute inset-y-0 left-0 z-10 flex w-1/2 flex-row items-center gap-4 bg-gradient-to-r from-white from-30% via-white/85 via-65% to-transparent">
            <h1 className="text-2xl font-bold">{gameTypeName}</h1>
          </div>
        </div>
      </header>

      <LayoutTabs
        gameTypeName={gameTypeName}
        playerCount={gameType.playerCount}
        clanCount={gameType.clanCount}
        serverCount={gameType.gameServerCount}
      />

      {children}
    </div>
  );
}
