import { notFound } from 'next/navigation';
import Link from 'next/link';
import prisma from '../../../../../utils/prisma';
import { LayoutTabs } from '../../LayoutTabs';
import { paramsSchema } from './schema';
import { encodeString } from '../../../../../utils/encoding';
import { ActivityCalendarSection } from '../../../../../components/ActivityCalendarSection';
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
        <div className="relative">
          <ActivityCalendarSection
            apiPath={`/api/gametype/${encodeString(gameTypeName)}/map/${encodeString(mapName)}/activity`}
            initial={activity}
          />
          <div className="absolute inset-y-0 left-0 z-10 flex w-1/2 flex-col justify-center gap-2 bg-gradient-to-r from-white from-30% via-white/85 via-65% to-transparent">
            <h1 className="text-2xl font-bold">{mapName}</h1>
            <span>
              <Link
                className="hover:underline"
                href={{ pathname: `/gametype/${encodeString(gameTypeName)}` }}
              >
                {gameTypeName}
              </Link>
            </span>
          </div>
        </div>
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
