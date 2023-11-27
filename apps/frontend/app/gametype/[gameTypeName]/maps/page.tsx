import prisma from '@teerank/frontend/utils/prisma';
import { paramsSchema, searchParamsSchema } from '../schema';
import { PlayerList } from '@teerank/frontend/components/PlayerList';
import { notFound } from 'next/navigation';
import { List, ListCell } from '@teerank/frontend/components/List';
import { formatInteger } from '@teerank/frontend/utils/format';

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

  const maps = await prisma.map.findMany({
    select: {
      name: true,
      gameTypeName: true,
      _count: {
        select: {
          playerInfos: true,
          clanInfos: true,
          snapshots: {
            where: {
              gameServerLast: {
                isNot: null,
              },
            },
          },
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
        playerInfos: {
          _count: 'desc',
        },
      },
    ],
    take: 100,
    skip: (page - 1) * 100,
  });

  const mapCount = await prisma.map.count({
    where: {
      gameType: {
        name: { equals: gameTypeName },
      },
    },
  });

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
              label={formatInteger(map._count.playerInfos)}
              href={{
                pathname: `/gametype/${encodeURIComponent(
                  map.gameTypeName
                )}/map/${encodeURIComponent(map.name)}`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(map._count.clanInfos)}
              href={{
                pathname: `/gametype/${encodeURIComponent(
                  map.gameTypeName
                )}/map/${encodeURIComponent(map.name)}/clans`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(map._count.snapshots)}
              href={{
                pathname: `/gametype/${encodeURIComponent(
                  map.gameTypeName
                )}/map/${encodeURIComponent(map.name)}/servers`,
              }}
            />
          </>
        ))}
      </List>
    </div>
  );
}
