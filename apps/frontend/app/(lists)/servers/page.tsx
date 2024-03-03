import { ServerList } from '../../../components/ServerList';
import { getGlobalCounts } from '../../../utils/globalCounts';
import prisma from '../../../utils/prisma';
import { searchParamSchema } from '../schema';
import { ComponentProps } from 'react';

export const metadata = {
  title: 'Servers',
  description: 'List of all servers',
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamSchema.parse(searchParams);

  const gameServers = await prisma.gameServer.findMany({
    where: {
      lastSnapshot: {
        isNot: null,
      },
    },
    orderBy: {
      lastSnapshot: {
        numClients: 'desc',
      },
    },
    include: {
      lastSnapshot: {
        include: {
          clients: true,
          map: true,
        },
      },
    },
    take: 100,
    skip: (page - 1) * 100,
  });

  const { gameServerCount } = await getGlobalCounts();

  const servers = gameServers.reduce<
    ComponentProps<typeof ServerList>['servers']
  >((arr, gameServer) => {
    if (gameServer.lastSnapshot !== null) {
      arr.push({
        rank: (page - 1) * 100 + arr.length + 1,
        name: gameServer.lastSnapshot.name,
        gameTypeName: gameServer.lastSnapshot.map.gameTypeName,
        mapName: gameServer.lastSnapshot.map.name,
        numClients: gameServer.lastSnapshot.numClients,
        maxClients: gameServer.lastSnapshot.maxClients,
        ip: gameServer.ip,
        port: gameServer.port,
      });
    }

    return arr;
  }, []);

  return <ServerList serverCount={gameServerCount} servers={servers} />;
}
