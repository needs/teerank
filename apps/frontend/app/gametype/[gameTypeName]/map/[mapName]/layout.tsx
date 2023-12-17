import prisma from '../../../../../utils/prisma';
import { LayoutTabs } from '../../LayoutTabs';
import { paramsSchema } from './schema';

export default async function Index({
  children,
  params,
}: {
  children: React.ReactNode;
  params: { [key: string]: string };
}) {
  const { gameTypeName, mapName } = paramsSchema.parse(params);

  const playerCount = await prisma.playerInfoMap.count({
    where: {
      map: {
        name: { equals: mapName },
        gameTypeName: { equals: gameTypeName },
      },
    },
  });

  const clanCount = await prisma.clanInfoMap.count({
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
      <LayoutTabs gameTypeName={gameTypeName} mapName={mapName} playerCount={playerCount} clanCount={clanCount} serverCount={serverCount} />

      {children}
    </div>
  );
}
