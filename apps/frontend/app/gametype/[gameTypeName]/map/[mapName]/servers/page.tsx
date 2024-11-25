import { ServerList } from '../../../../../../components/ServerList';
import prisma from '../../../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from '../schema';

export const metadata = {
  title: 'Servers',
  description: 'List of ranked servers',
};

export default async function Index({
  params,
  searchParams,
}: {
  params: { [key: string]: string };
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamsSchema.parse(searchParams);
  const { gameTypeName, mapName } = paramsSchema.parse(params);

  const gameServerStates = await prisma.gameServerState.findMany({
    select: {
      name: true,
      numClients: true,
      maxClients: true,
      map: {
        select: {
          name: true,
          gameTypeName: true,
        },
      },
      gameServer: {
        select: {
          ip: true,
          port: true,
        },
      },
    },
    where: {
      map: {
        name: {
          equals: mapName,
        },
        gameTypeName: {
          equals: gameTypeName,
        },
      },
    },
    orderBy: [
      {
        numClients: 'desc',
      },
      {
        maxClients: 'desc',
      },
    ],
    take: 100,
    skip: (page - 1) * 100,
  });

  const serverCount = await prisma.gameServerState.count({
    where: {
      map: {
        name: {
          equals: mapName,
        },
        gameTypeName: {
          equals: gameTypeName,
        },
      },
    },
  });

  return (
    <ServerList
      serverCount={serverCount}
      servers={gameServerStates.map((gameServerState, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: gameServerState.name,
        gameTypeName: gameServerState.map.gameTypeName,
        mapName: gameServerState.map.name ?? '',
        numClients: gameServerState.numClients,
        maxClients: gameServerState.maxClients,
        ip: gameServerState.gameServer?.ip ?? '',
        port: gameServerState.gameServer?.port ?? 0,
      }))}
    />
  );
}
