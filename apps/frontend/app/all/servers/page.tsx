import { ServerList } from '../../../components/ServerList';
import { getGlobalCounts } from '../../../utils/globalCounts';
import prisma from '../../../utils/prisma';
import { searchParamSchema } from '../schema';
import { ComponentProps } from 'react';

export const metadata = {
  title: 'All Servers - Teerank',
  description: 'Teerank is a simple and fast ranking system for Teeworlds.',
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamSchema.parse(searchParams);

  const gameServers = await prisma.gameServer.findMany({
    where: {
      gameServerStateId: {
        not: null,
      },
    },
    orderBy: {
      gameServerState: {
        numClients: 'desc',
      },
    },
    include: {
      gameServerState: {
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
    if (gameServer.gameServerState !== null) {
      arr.push({
        rank: (page - 1) * 100 + arr.length + 1,
        name: gameServer.gameServerState.name,
        gameTypeName: gameServer.gameServerState.map.gameTypeName,
        mapName: gameServer.gameServerState.map.name,
        numClients: gameServer.gameServerState.numClients,
        maxClients: gameServer.gameServerState.maxClients,
        ip: gameServer.ip,
        port: gameServer.port,
      });
    }

    return arr;
  }, []);

  return <ServerList serverCount={gameServerCount} servers={servers} />;
}
