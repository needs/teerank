import prisma from '@teerank/frontend/utils/prisma';
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

  const playerCount = await prisma.playerInfo.count({
    where: {
      gameType: {
        name: { equals: gameTypeName },
      },
    },
  });

  const clanCount = await prisma.clanInfo.count({
    where: {
      gameType: {
        name: { equals: gameTypeName },
      },
    },
  });

  const serverCount = await prisma.gameServer.count({
    where: {
      AND: [
        { lastSnapshot: { isNot: null } },
        {
          lastSnapshot: {
            map: {
              gameTypeName: { equals: gameTypeName },
            },
          },
        },
      ],
    },
  });

  return (
    <div className="flex flex-col gap-4 py-8">
      <LayoutTabs gameTypeName={gameTypeName} playerCount={playerCount} clanCount={clanCount} serverCount={serverCount} />

      {children}
    </div>
  );
}
