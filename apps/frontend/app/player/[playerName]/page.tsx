import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import Link from 'next/link';
import { Metadata } from 'next';
import { Fragment } from 'react';
import { searchParamPageSchema } from '../../../utils/page';
import prisma from '../../../utils/prisma';
import { List, ListCell } from '../../../components/List';
import { encodeString } from '../../../utils/encoding';
import { formatPlayTime } from '../../../utils/format';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { playerName } = paramsSchema.parse(params);

  return {
    title: `Player ${playerName}`,
    description: 'A Teeworlds player',
    alternates: {
      canonical: `https://teerank.io/player/${encodeString(playerName)}`,
    },
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
  const showMaps = searchParams['show'] === 'maps';

  const player = await prisma.player.findUnique({
    select: {
      _count: {
        select: {
          playerInfoGameTypes: true,
          playerInfoMaps: true,
        },
      },

      playerInfoGameTypes: {
        select: {
          gameTypeName: true,
          playTime: true,
        },
        orderBy: {
          playTime: 'desc',
        },
        take: showMaps ? 0 : 100,
        skip: showMaps ? 0 : (page - 1) * 100,
      },

      playerInfoMaps: {
        select: {
          map: {
            select: {
              name: true,
              gameTypeName: true,
            },
          },
          playTime: true,
        },
        orderBy: {
          playTime: 'desc',
        },
        take: showMaps ? 100 : 0,
        skip: showMaps ? (page - 1) * 100 : 0,
      },
    },
    where: {
      name: playerName,
    },
  });

  if (player === null) {
    return notFound();
  }

  const pathname = `/player/${encodeString(playerName)}`;

  const rows = showMaps
    ? player.playerInfoMaps.map((playerInfoMap) => ({
        gameTypeName: playerInfoMap.map?.gameTypeName ?? '',
        mapName: playerInfoMap.map?.name ?? '',
        playTime: playerInfoMap.playTime,
      }))
    : player.playerInfoGameTypes.map((playerInfoGameType) => ({
        gameTypeName: playerInfoGameType.gameTypeName ?? '',
        mapName: undefined,
        playTime: playerInfoGameType.playTime,
      }));

  const count = showMaps
    ? player._count.playerInfoMaps
    : player._count.playerInfoGameTypes;

  return (
    <List
      columns={[
        {
          title: 'Game type',
          expand: true,
        },
        {
          title: (
            <>
              <span className={showMaps ? undefined : 'text-[#999]'}>Map</span>
              <Link
                prefetch={false}
                href={
                  showMaps ? { pathname } : { pathname, query: { show: 'maps' } }
                }
                className="ml-3 rounded border border-[#970] px-2 align-middle text-sm font-normal hover:bg-[#970] hover:text-white"
              >
                {showMaps ? 'Hide' : 'Show'}
              </Link>
            </>
          ),
          expand: true,
        },
        {
          title: 'Playtime',
          expand: false,
        },
      ]}
      pageCount={Math.ceil(count / 100)}
    >
      {rows.map((row, index) => (
        <Fragment key={`${row.gameTypeName}\0${row.mapName ?? index}`}>
          <ListCell
            label={row.gameTypeName}
            href={{
              pathname: `/gametype/${encodeString(row.gameTypeName)}`,
            }}
          />
          <ListCell
            label={row.mapName}
            href={
              row.mapName === undefined
                ? undefined
                : {
                    pathname: `/gametype/${encodeString(
                      row.gameTypeName
                    )}/map/${encodeString(row.mapName)}`,
                  }
            }
          />
          <ListCell alignRight label={formatPlayTime(row.playTime)} />
        </Fragment>
      ))}
    </List>
  );
}
