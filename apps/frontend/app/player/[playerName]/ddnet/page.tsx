import { paramsSchema } from '../schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import Link from 'next/link';
import { Metadata } from 'next';
import { Fragment } from 'react';
import prisma from '../../../../utils/prisma';
import { searchParamPageSchema } from '../../../../utils/page';
import { List, ListCell } from '../../../../components/List';
import { DDNetAttribution } from '../../../../components/DDNetAttribution';
import { encodeString } from '../../../../utils/encoding';
import { formatFinishTime, formatInteger } from '../../../../utils/format';
import {
  countDdnetBestTimes,
  formatUtcDate,
  getDdnetBestTimes,
  getDdnetJourney,
  getDdnetPlayer,
} from '../../../../utils/ddnetPlayer';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { playerName } = paramsSchema.parse(params);

  return {
    title: `Player ${playerName} - DDNet`,
    description: 'DDNet finishes and best times of a Teeworlds player',
    alternates: {
      canonical: `https://teerank.io/player/${encodeString(playerName)}/ddnet`,
    },
  };
}

function mapHref(mapName: string) {
  return {
    pathname: `/gametype/DDraceNetwork/map/${encodeString(mapName)}`,
  };
}

export default async function Index({
  params,
  searchParams,
}: {
  params: z.infer<typeof paramsSchema>;
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { playerName } = paramsSchema.parse(params);
  const { page } = searchParamPageSchema.parse(searchParams);

  const player = await prisma.player.findUnique({
    select: {
      id: true,
    },
    where: {
      name: playerName,
    },
  });

  if (player === null) {
    return notFound();
  }

  const ddnetPlayer = await getDdnetPlayer(player.id);

  if (ddnetPlayer === null) {
    return notFound();
  }

  const [journey, bestTimes, bestTimeCount] = await Promise.all([
    getDdnetJourney(player.id, ddnetPlayer.lastFinishAt),
    getDdnetBestTimes(player.id, { skip: (page - 1) * 100, take: 100 }),
    countDdnetBestTimes(player.id),
  ]);

  return (
    <>
      <section className="flex flex-col gap-2">
        <h2 className="px-4 lg:px-8 xl:px-16 text-xl font-bold">Journey</h2>
        <List
          columns={[
            { title: 'Tier', expand: true },
            { title: 'First finish', expand: true },
            { title: 'Date', expand: false },
            { title: 'Finishes', expand: false },
            { title: 'Points', expand: false },
          ]}
        >
          {journey.tiers.map((tier) => (
            <Fragment key={tier.category}>
              <ListCell label={tier.category} />
              <ListCell
                label={tier.firstMapName}
                href={mapHref(tier.firstMapName)}
              />
              <ListCell alignRight label={formatUtcDate(tier.firstFinishAt)} />
              <ListCell alignRight label={formatInteger(tier.finishCount)} />
              <ListCell alignRight label={formatInteger(tier.points)} />
            </Fragment>
          ))}
        </List>
        <p className="px-4 lg:px-8 xl:px-16 text-sm text-[#999]">
          {'First finish: '}
          {journey.firstMapName !== null && (
            <>
              <Link
                prefetch={false}
                className="hover:underline"
                href={mapHref(journey.firstMapName)}
              >
                {journey.firstMapName}
              </Link>
              {' · '}
            </>
          )}
          {formatUtcDate(ddnetPlayer.firstFinishAt, 'MMM yyyy')}
          {' — Last finish: '}
          {journey.lastMapName !== null && (
            <>
              <Link
                prefetch={false}
                className="hover:underline"
                href={mapHref(journey.lastMapName)}
              >
                {journey.lastMapName}
              </Link>
              {' · '}
            </>
          )}
          {formatUtcDate(ddnetPlayer.lastFinishAt, 'MMM yyyy')}
        </p>
      </section>

      <section className="flex flex-col gap-2">
        <h2 className="px-4 lg:px-8 xl:px-16 text-xl font-bold">Best times</h2>
        <List
          columns={[
            { title: 'Map', expand: true },
            { title: 'Tier', expand: true },
            { title: 'Time', expand: false },
            { title: 'Rank', expand: false },
            { title: 'Top %', expand: false },
            { title: 'Date', expand: false },
            { title: '', expand: false },
          ]}
          pageCount={Math.ceil(bestTimeCount / 100)}
        >
          {bestTimes.map((bestTime) => (
            <Fragment key={bestTime.mapId}>
              <ListCell
                label={bestTime.mapName ?? '—'}
                href={
                  bestTime.mapName === null
                    ? undefined
                    : mapHref(bestTime.mapName)
                }
              />
              <ListCell label={bestTime.category ?? '—'} />
              <ListCell alignRight label={formatFinishTime(bestTime.time)} />
              <ListCell alignRight label={formatInteger(bestTime.rank)} />
              <ListCell
                alignRight
                label={
                  bestTime.topPercent === null
                    ? '—'
                    : `top ${bestTime.topPercent}%`
                }
              />
              <ListCell alignRight label={formatUtcDate(bestTime.bestAt)} />
              <ListCell
                alignRight
                className="text-sm"
                label={
                  bestTime.hasSplits && bestTime.mapName !== null
                    ? 'splits'
                    : undefined
                }
                href={
                  bestTime.mapName === null
                    ? undefined
                    : {
                        pathname: `/gametype/DDraceNetwork/map/${encodeString(
                          bestTime.mapName
                        )}/records`,
                        query: { compare: playerName },
                      }
                }
              />
            </Fragment>
          ))}
        </List>
        <DDNetAttribution />
      </section>
    </>
  );
}
