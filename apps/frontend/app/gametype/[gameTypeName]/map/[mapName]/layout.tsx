import { notFound } from 'next/navigation';
import prisma from '../../../../../utils/prisma';
import { LayoutTabs } from '../../LayoutTabs';
import { paramsSchema } from './schema';
import { updateMapCountsIfOutdated } from "@teerank/teerank";

export default async function Index({
  children,
  params,
}: {
  children: React.ReactNode;
  params: { [key: string]: string };
}) {
  const { gameTypeName, mapName } = paramsSchema.parse(params);

  let map = await prisma.map.findUnique({
    where: {
      name_gameTypeName: {
        name: mapName,
        gameTypeName: gameTypeName,
      },
    },
  });

  if (map === null) {
    notFound();
  }

  map = await updateMapCountsIfOutdated(prisma, map);

  return (
    <div className="flex flex-col gap-4 py-8">
      <LayoutTabs gameTypeName={gameTypeName} mapName={mapName} playerCount={map.playerCount} clanCount={map.clanCount} serverCount={map.gameServerCount} />

      {children}
    </div>
  );
}
