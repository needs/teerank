import groupBy from 'lodash.groupby';
import { searchParamSchema } from '../(lists)/schema';
import { List, ListCell } from '../../components/List';
import { capitalize, formatInteger, formatPlayTime } from '../../utils/format';
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

  const [onlineGameServers, gameTypes, gameTypeCount] = await Promise.all([
    prisma.gameServer.findMany({
      select: {
        lastSnapshot: {
          select: {
            map: {
              select: {
                gameType: true,
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

    prisma.gameType.findMany({
      select: {
        name: true,
        rankMethod: true,
        playTime: true,
        _count: {
          select: {
            playerInfoGameTypes: true,
            map: true,
            clanInfoGameTypes: true,
          },
        },
      },
      orderBy: [
        {
          playerInfoGameTypes: {
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
    }),

    prisma.gameType.count()
  ]);

  const onlineGameServersByGameType = groupBy(
    onlineGameServers,
    (server) => server.lastSnapshot?.map.gameType.name
  );

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
            title: 'Ranking',
            expand: false,
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
          {
            title: 'Play Time',
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
              label={capitalize(gameType.rankMethod ?? '')}
            />
            <ListCell
              alignRight
              label={formatInteger(gameType._count.playerInfoGameTypes)}
              href={{
                pathname: `/gametype/${encodeURIComponent(gameType.name)}`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(gameType._count.clanInfoGameTypes)}
              href={{
                pathname: `/gametype/${encodeURIComponent(
                  gameType.name
                )}/clans`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(
                onlineGameServersByGameType[gameType.name]?.length ?? 0
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
            <ListCell alignRight label={formatPlayTime(gameType.playTime)} />
          </>
        ))}
      </List>
    </div>
  );
}
