import { ServerList } from '../../../components/ServerList';
import { getGlobalCounts } from '@teerank/teerank';
import prisma from '../../../utils/prisma';
import { searchParamSchema } from '../schema';
import { ComponentProps } from 'react';
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

  const gameServerStates = await prisma.gameServerState.findMany({
    orderBy: {
      numClients: 'desc',
    },
    include: {
      gameServer: true,
      map: true,
    },
    take: 100,
    skip: (page - 1) * 100,
  });

  const globalCounts = await getGlobalCounts(redis);

  const servers = gameServerStates.map((gameServerState, index) => ({
    rank: (page - 1) * 100 + index + 1,
    name: gameServerState.name,
    gameTypeName: gameServerState.map.gameTypeName,
    mapName: gameServerState.map.name,
    numClients: gameServerState.numClients,
    maxClients: gameServerState.maxClients,
    ip: gameServerState.gameServer.ip,
    port: gameServerState.gameServer.port,
  }));

  return (
    <ServerList
      serverCount={globalCounts.gameServers}
      servers={servers}
    />
  );
}
