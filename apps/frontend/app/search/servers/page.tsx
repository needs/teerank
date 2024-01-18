import { searchParamSchema } from '../schema';
import { LayoutTabs } from '../LayoutTabs';
import uniqBy from 'lodash.uniqby';
import { Error } from '../SearchError';
import { ServerList } from '../../../components/ServerList';
import prisma from '../../../utils/prisma';

export const metadata = {
  title: 'Search - Servers',
  description: 'Search for servers',
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { query } = searchParamSchema.parse(searchParams);

  if (query.length < 2) {
    return <Error message="Please enter at least 2 characters." />;
  }

  const matches = await Promise.all([
    prisma.gameServerSnapshot.findMany({
      where: {
        gameServerLast: {
          isNot: null,
        },
        name: {
          equals: query,
          mode: 'insensitive',
        },
      },
      select: {
        name: true,
        map: true,
        numClients: true,
        maxClients: true,
        gameServerLast: {
          select: {
            id: true,
            ip: true,
            port: true,
          },
        },
      },
      orderBy: {
        name: 'asc',
      },
      take: 100,
    }),
    prisma.gameServerSnapshot.findMany({
      where: {
        gameServerLast: {
          isNot: null,
        },
        OR: [
          {
            name: {
              startsWith: query,
              mode: 'insensitive',
            },
          },
          {
            name: {
              endsWith: query,
              mode: 'insensitive',
            },
          },
        ],
      },
      select: {
        name: true,
        map: true,
        numClients: true,
        maxClients: true,
        gameServerLast: {
          select: {
            id: true,
            ip: true,
            port: true,
          },
        },
      },
      orderBy: {
        name: 'asc',
      },
      take: 100,
    }),
    prisma.gameServerSnapshot.findMany({
      where: {
        gameServerLast: {
          isNot: null
        },
        name: {
          contains: query,
          mode: 'insensitive',
        },
      },
      select: {
        name: true,
        map: true,
        numClients: true,
        maxClients: true,
        gameServerLast: {
          select: {
            id: true,
            ip: true,
            port: true,
          },
        },
      },
      orderBy: {
        name: 'asc',
      },
      take: 100,
    }),
  ]);

  const gameServerSnapshots = uniqBy(matches.flat(), 'gameServerLast.id').slice(0, 100);

  return (
    <LayoutTabs query={query} selectedTab="servers">
      {({ gameServerCount }) => (
        <ServerList
          serverCount={gameServerCount}
          servers={gameServerSnapshots.map((gameServerSnapshot, index) => ({
            rank: index + 1,
            name: gameServerSnapshot.name,
            gameTypeName: gameServerSnapshot.map.gameTypeName,
            mapName: gameServerSnapshot.map.name,
            ip: gameServerSnapshot.gameServerLast!.ip,
            port: gameServerSnapshot.gameServerLast!.port,
            numClients: gameServerSnapshot.numClients,
            maxClients: gameServerSnapshot.maxClients,
          }))}
        />
      )}
    </LayoutTabs>
  );
}
