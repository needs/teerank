import { notFound } from 'next/navigation';
import prisma from '../../../../utils/prisma';
import { LayoutTabs } from '../LayoutTabs';
import { paramsSchema } from '../schema';
import { updateGameTypeCountsIfOutdated } from '@teerank/teerank';

export default async function Index({
  children,
  params,
}: {
  children: React.ReactNode;
  params: { gameType: string };
}) {
  const { gameTypeName } = paramsSchema.parse(params);

  let gameType = await prisma.gameType.findUnique({
    where: {
      name: gameTypeName,
    },
  });

  if (gameType === null) {
    notFound();
  }

  gameType = await updateGameTypeCountsIfOutdated(prisma, gameType);

  return (
    <div className="flex flex-col gap-4 py-8">
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
