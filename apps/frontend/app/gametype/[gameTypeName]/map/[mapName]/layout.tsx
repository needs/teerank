import { notFound } from 'next/navigation';
import Link from 'next/link';
import prisma from '../../../../../utils/prisma';
import { LayoutTabs } from '../../LayoutTabs';
import { paramsSchema } from './schema';
import { encodeString } from '../../../../../utils/encoding';
import { ActivityHeader } from '../../../../../components/ActivityHeader';
import { getMapActivity } from '../../../../../utils/activity';

export default async function Index({
  children,
  params,
}: {
  children: React.ReactNode;
  params: { [key: string]: string };
}) {
  const { gameTypeName, mapName } = paramsSchema.parse(params);

  const map = await prisma.map.findUnique({
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

  const activity = await getMapActivity(map.id, { range: '1y' });

  return (
    <div className="flex flex-col gap-4 py-8">
      <header className="px-8 xl:px-20">
        <ActivityHeader
          apiPath={`/api/gametype/${encodeString(gameTypeName)}/map/${encodeString(mapName)}/activity`}
          activity={activity}
          contentClassName="flex-col justify-center gap-2"
        >
          <h1 className="text-2xl font-bold">{mapName}</h1>
          <span>
            <Link
              className="hover:underline"
              href={{ pathname: `/gametype/${encodeString(gameTypeName)}` }}
            >
              {gameTypeName}
            </Link>
          </span>
        </ActivityHeader>
      </header>

      <LayoutTabs
        gameTypeName={gameTypeName}
        mapName={mapName}
        playerCount={map.playerCount}
        clanCount={map.clanCount}
        serverCount={map.gameServerCount}
      />

      {children}
    </div>
  );
}
