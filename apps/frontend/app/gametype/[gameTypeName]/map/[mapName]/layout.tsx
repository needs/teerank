import prisma from '@teerank/frontend/utils/prisma';
import { LayoutTabs } from '@teerank/frontend/app/gametype/[gameTypeName]/LayoutTabs';
import { paramsSchema } from './schema';

export default async function Index({
  children,
  params,
}: {
  children: React.ReactNode;
  params: { [key: string]: string };
}) {
  const { gameTypeName, mapName } = paramsSchema.parse(params);

  const playerCount = await prisma.playerInfo.count({
    where: {
      map: {
        name: { equals: mapName },
        gameTypeName: { equals: gameTypeName },
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
              name: { equals: mapName },
              gameTypeName: { equals: gameTypeName },
            },
          },
        },
      ],
    },
  });

  return (
    <div className="flex flex-col gap-4 py-8">
      <LayoutTabs gameTypeName={gameTypeName} mapName={mapName} playerCount={playerCount} serverCount={serverCount} />

      {children}
    </div>
  );
}
