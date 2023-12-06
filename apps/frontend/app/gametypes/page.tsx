import { searchParamSchema } from '../(lists)/schema';
import { List, ListCell } from '../../components/List';
import { formatInteger } from '../../utils/format';
import prisma from '../../utils/prisma';

export const metadata = {
  title: 'Gametypes',
  description: 'List of all gametypes',
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamSchema.parse(searchParams);

  const gameTypes = await prisma.gameType.findMany({
    select: {
      name: true,
      _count: {
        select: {
          playerInfos: true,
          map: true,
          clanInfos: true,
        },
      },
      map: {
        select: {
          _count: {
            select: {
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
      },
    },
    orderBy: [
      {
        playerInfos: {
          _count: 'desc',
        },
      },
      {
        map: {
          _count: 'desc',
        },
      },
    ],
    take: 100,
    skip: (page - 1) * 100,
  });

  const gameTypeCount = await prisma.gameType.count();

  return (
    <div className="py-8">
      <List
        pageCount={Math.ceil(gameTypeCount / 100)}
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
            title: 'Maps',
            expand: false,
          },
        ]}
      >
        {gameTypes.map((gameType, index) => (
          <>
            <ListCell
              alignRight
              label={formatInteger((page - 1) * 100 + index + 1)}
            />
            <ListCell
              label={gameType.name}
              href={{
                pathname: `/gametype/${encodeURIComponent(gameType.name)}`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(gameType._count.playerInfos)}
              href={{
                pathname: `/gametype/${encodeURIComponent(gameType.name)}`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(gameType._count.clanInfos)}
              href={{
                pathname: `/gametype/${encodeURIComponent(
                  gameType.name
                )}/clans`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(
                gameType.map.reduce((sum, map) => sum + map._count.snapshots, 0)
              )}
              href={{
                pathname: `/gametype/${encodeURIComponent(
                  gameType.name
                )}/servers`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(gameType._count.map)}
              href={{
                pathname: `/gametype/${encodeURIComponent(gameType.name)}/maps`,
              }}
            />
          </>
        ))}
      </List>
    </div>
  );
}
