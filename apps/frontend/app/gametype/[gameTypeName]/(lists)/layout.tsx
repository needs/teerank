import { notFound } from 'next/navigation';
import prisma from '../../../../utils/prisma';
import { LayoutTabs } from '../LayoutTabs';
import { paramsSchema } from '../schema';

export default async function Index({
  children,
  params,
}: {
  children: React.ReactNode;
  params: { gameType: string };
}) {
  const { gameTypeName } = paramsSchema.parse(params);

  const gameType = await prisma.gameType.findUnique({
    select: {
      playerCount: true,
      clanCount: true,
      gameServerCount: true,
    },
    where: {
      name: gameTypeName,
    },
  });

  if (gameType === null) {
    notFound();
  }

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
