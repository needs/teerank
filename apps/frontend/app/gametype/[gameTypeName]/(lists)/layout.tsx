import { notFound } from 'next/navigation';
import prisma from '../../../../utils/prisma';
import { LayoutTabs } from '../LayoutTabs';
import { paramsSchema } from '../schema';
import { encodeString } from '../../../../utils/encoding';
import { ActivityHeader } from '../../../../components/ActivityHeader';
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
        <ActivityHeader
          apiPath={`/api/gametype/${encodeString(gameTypeName)}/activity`}
          activity={activity}
          contentClassName="flex-row items-center gap-4"
        >
          <h1 className="text-2xl font-bold">{gameTypeName}</h1>
        </ActivityHeader>
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
