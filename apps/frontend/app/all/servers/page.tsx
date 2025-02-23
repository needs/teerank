import { ServerList } from '../../../components/ServerList';
import { getGlobalCounts } from '@teerank/teerank';
import prisma from '../../../utils/prisma';
import { searchParamSchema } from '../schema';
import redis from '../../../utils/redis';

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
    select: {
      ip: true,
      port: true,
      gameServerState: {
        select: {
          name: true,
          numClients: true,
          maxClients: true,
          map: {
            select: {
              gameTypeName: true,
              name: true,
            },
          },
        },
      },
    },
    orderBy: [
      {
        failureCount: 'asc',
      },
      {
        gameServerState: {
          maxClients: 'desc',
        },
      },
      {
        gameServerState: {
          numClients: 'desc',
        },
      },
      {
        playTime: 'desc',
      },
    ],
    take: 100,
    skip: (page - 1) * 100,
  });

  const globalCounts = await getGlobalCounts(redis);

  const servers = gameServers.map((gameServer, index) => ({
    rank: (page - 1) * 100 + index + 1,
    ip: gameServer.ip,
    port: gameServer.port,
    state: gameServer.gameServerState
      ? {
          name: gameServer.gameServerState.name,
          gameTypeName: gameServer.gameServerState.map.gameTypeName,
          mapName: gameServer.gameServerState.map.name,
          numClients: gameServer.gameServerState.numClients,
          maxClients: gameServer.gameServerState.maxClients,
        }
      : null,
  }));

  return (
    <ServerList serverCount={globalCounts.gameServers} servers={servers} />
  );
}
