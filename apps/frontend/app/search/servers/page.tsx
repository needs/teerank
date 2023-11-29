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

  const matches = await prisma.$transaction([
    prisma.gameServer.findMany({
      where: {
        lastSnapshot: {
          name: {
            equals: query,
            mode: 'insensitive',
        },
        },
      },
      select: {
        id: true,
        ip: true,
        port: true,
        lastSnapshot: {
          select: {
            name: true,
            map: true,
            numClients: true,
            maxClients: true,
          },
        },
      },
      orderBy: {
        lastSnapshot: {
          name: 'asc',
        },
      },
      take: 100,
    }),
    prisma.gameServer.findMany({
      where: {
        lastSnapshot: {
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
      },
      select: {
        id: true,
        ip: true,
        port: true,
        lastSnapshot: {
          select: {
            name: true,
            map: true,
            numClients: true,
            maxClients: true,
          },
        },
      },
      orderBy: {
        lastSnapshot: {
          name: 'asc',
        },
      },
      take: 100,
    }),
    prisma.gameServer.findMany({
      where: {
        lastSnapshot: {
          name: {
            contains: query,
            mode: 'insensitive',
          },
        },
      },
      select: {
        id: true,
        ip: true,
        port: true,
        lastSnapshot: {
          select: {
            name: true,
            map: true,
            numClients: true,
            maxClients: true,
          },
        },
      },
      orderBy: {
        lastSnapshot: {
          name: 'asc',
        },
      },
      take: 100,
    }),
  ]);

  const gameServers = uniqBy(matches.flat(), 'id').slice(0, 100);

  return (
    <LayoutTabs query={query} selectedTab="servers">
      {({ gameServerCount }) => (
        <ServerList
          serverCount={gameServerCount}
          servers={gameServers.map((gameServer, index) => ({
            rank: index + 1,
            name: gameServer.lastSnapshot!.name,
            gameTypeName: gameServer.lastSnapshot!.map.gameTypeName,
            mapName: gameServer.lastSnapshot!.map.name,
            ip: gameServer.ip,
            port: gameServer.port,
            numClients: gameServer.lastSnapshot!.numClients,
            maxClients: gameServer.lastSnapshot!.maxClients,
          }))}
        />
      )}
    </LayoutTabs>
  );
}
