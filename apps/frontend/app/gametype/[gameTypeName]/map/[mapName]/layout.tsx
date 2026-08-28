import { notFound } from 'next/navigation';
import Link from 'next/link';
import { Fragment } from 'react';
import { format } from 'date-fns';
import { localizeUtcDay } from '@teerank/teerank/date';
import prisma from '../../../../../utils/prisma';
import { LayoutTabs } from '../../LayoutTabs';
import { paramsSchema } from './schema';
import { encodeString } from '../../../../../utils/encoding';
import { ActivityHeader } from '../../../../../components/ActivityHeader';
import { getMapActivity } from '../../../../../utils/activity';
import { getDdnetMapHeader } from '../../../../../utils/ddnetMap';
import { formatInteger } from '../../../../../utils/format';

function MapperLinks({ ddnetMap, mappers }: { ddnetMap: { mapper: string }; mappers: string[] }) {
  if (mappers.length === 0) {
    return <>{ddnetMap.mapper}</>;
  }

  return (
    <>
      {mappers.map((mapper, index) => (
        <Fragment key={mapper}>
          {index > 0 && ', '}
          <Link
            prefetch={false}
            className="hover:underline"
            href={{ pathname: `/mapper/${encodeString(mapper)}` }}
          >
            {mapper}
          </Link>
        </Fragment>
      ))}
    </>
  );
}

function DdnetMapStats({
  ddnetMap,
  recordsPath,
}: {
  ddnetMap: {
    category: string;
    points: number;
    releasedAt: Date | null;
    finishCount: number;
    tiles: string[];
  };
  recordsPath: string;
}) {
  const tiles = ddnetMap.tiles.map((tile) => tile.toLowerCase().replace(/_/g, ' '));

  return (
    <>
      <span
        className={`px-4 ${tiles.length > 0 ? 'cursor-help' : ''}`}
        title={tiles.length > 0 ? tiles.join(', ') : undefined}
      >
        {ddnetMap.category} · {ddnetMap.points} pts
      </span>
      {ddnetMap.releasedAt !== null && (
        <span className="px-4">
          Released {format(localizeUtcDay(ddnetMap.releasedAt), 'MMM yyyy')}
        </span>
      )}
      <span className="px-4">
        <Link prefetch={false} className="hover:underline" href={{ pathname: recordsPath }}>
          {formatInteger(ddnetMap.finishCount)} finishes
        </Link>
      </span>
    </>
  );
}

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

  const [activity, ddnet] = await Promise.all([
    getMapActivity(map.id, { range: '1y' }),
    gameTypeName === 'DDraceNetwork' ? getDdnetMapHeader(map.id) : null,
  ]);

  return (
    <div className="flex flex-col gap-4 py-8">
      <header className="px-8 xl:px-20">
        <ActivityHeader
          apiPath={`/api/gametype/${encodeString(gameTypeName)}/map/${encodeString(mapName)}/activity`}
          activity={activity}
          contentClassName="flex-col justify-center gap-2"
        >
          <section className="flex flex-row items-center gap-6">
            {ddnet !== null && ddnet.hasThumb && (
              <img
                src={`/map-thumb/${map.id}`}
                width={144}
                height={90}
                alt={`${mapName} thumbnail`}
                className="hidden rounded sm:block"
              />
            )}
            <div className="flex flex-col justify-center gap-2">
              <h1 className="flex flex-row flex-wrap items-baseline gap-x-3">
                <span className="text-2xl font-bold">{mapName}</span>
                {ddnet !== null && ddnet.ddnetMap.stars > 0 && (
                  <span className="text-[#970]" title={`${ddnet.ddnetMap.stars} stars`}>
                    {'★'.repeat(ddnet.ddnetMap.stars)}
                  </span>
                )}
                {ddnet !== null && (
                  <span className="text-base font-normal">
                    by <MapperLinks ddnetMap={ddnet.ddnetMap} mappers={ddnet.mappers} />
                  </span>
                )}
              </h1>
              <div className="flex flex-row divide-x">
                <span className="pr-4">
                  <Link
                    className="hover:underline"
                    href={{ pathname: `/gametype/${encodeString(gameTypeName)}` }}
                  >
                    {gameTypeName}
                  </Link>
                </span>
                {ddnet !== null && (
                  <DdnetMapStats
                    ddnetMap={ddnet.ddnetMap}
                    recordsPath={`/gametype/${encodeString(gameTypeName)}/map/${encodeString(mapName)}/records`}
                  />
                )}
              </div>
            </div>
          </section>
        </ActivityHeader>
      </header>

      <LayoutTabs
        gameTypeName={gameTypeName}
        mapName={mapName}
        playerCount={map.playerCount}
        clanCount={map.clanCount}
        serverCount={map.gameServerCount}
        showRecords={ddnet !== null}
      />

      {children}
    </div>
  );
}
