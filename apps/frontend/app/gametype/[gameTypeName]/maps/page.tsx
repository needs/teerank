import groupBy from 'lodash.groupby';
import { List, ListCell } from '../../../../components/List';
import { formatInteger, formatPlayTime } from '../../../../utils/format';
import prisma from '../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from '../schema';
import { notFound } from 'next/navigation';

export const metadata = {
  title: 'Maps',
  description: 'List of gametype maps',
};

export default async function Index({
  params,
  searchParams,
}: {
  params: { [key: string]: string };
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamsSchema.parse(searchParams);
  const { gameTypeName } = paramsSchema.parse(params);

  const gameType = await prisma.gameType.findUnique({
    where: {
      name: gameTypeName,
    },
  });

  if (gameType === null) {
    return notFound();
  }

  const [onlineGameServers, maps, mapCount] = await Promise.all([
    prisma.gameServer.findMany({
      select: {
        lastSnapshot: {
          select: {
            map: {
              select: {
                name: true,
              },
            },
          },
        },
      },
      where: {
        lastSnapshot: {
          isNot: null,
        },
      },
    }),

    prisma.map.findMany({
      select: {
        name: true,
        gameTypeName: true,
        playTime: true,
        _count: {
          select: {
            playerInfoMaps: true,
            clanInfoMaps: true,
          },
        },
      },
      where: {
        gameType: {
          name: { equals: gameTypeName },
        },
      },
      orderBy: [
        {
          playerInfoMaps: {
            _count: 'desc',
          },
        },
      ],
      take: 100,
      skip: (page - 1) * 100,
    }),

    prisma.map.count({
      where: {
        gameType: {
          name: { equals: gameTypeName },
        },
      },
    }),
  ]);

  const onlineGameServersByMap = groupBy(
    onlineGameServers,
    (server) => server.lastSnapshot?.map.name
  );

  return (
    <div className="py-8">
      <List
        columns={[
          {
            title: '',
            expand: false,
          },
          {
            title: 'Name',
            expand: true,
          },
          {
            title: 'Players',
            expand: false,
          },
          {
            title: 'Clans',
            expand: false,
          },
          {
            title: 'Servers',
            expand: false,
          },
          {
            title: 'Play Time',
            expand: false,
          },
        ]}
        pageCount={Math.ceil(mapCount / 100)}
      >
        {maps.map((map, index) => (
          <>
            <ListCell
              alignRight
              label={formatInteger((page - 1) * 100 + index + 1)}
            />
            <ListCell
              label={map.name}
              href={{
                pathname: `/gametype/${encodeURIComponent(
                  map.gameTypeName
                )}/map/${encodeURIComponent(map.name)}`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(map._count.playerInfoMaps)}
              href={{
                pathname: `/gametype/${encodeURIComponent(
                  map.gameTypeName
                )}/map/${encodeURIComponent(map.name)}`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(map._count.clanInfoMaps)}
              href={{
                pathname: `/gametype/${encodeURIComponent(
                  map.gameTypeName
                )}/map/${encodeURIComponent(map.name)}/clans`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(
                onlineGameServersByMap[map.name]?.length ?? 0
              )}
              href={{
                pathname: `/gametype/${encodeURIComponent(
                  map.gameTypeName
                )}/map/${encodeURIComponent(map.name)}/servers`,
              }}
            />
            <ListCell alignRight label={formatPlayTime(map.playTime)} />
          </>
        ))}
      </List>
    </div>
  );
}
