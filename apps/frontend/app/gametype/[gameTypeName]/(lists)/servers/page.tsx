import prisma from '@teerank/frontend/utils/prisma';
import { paramsSchema, searchParamsSchema } from '../../schema';
import { ServerList } from '@teerank/frontend/components/ServerList';
import { ComponentProps } from 'react';

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
  const { gameTypeName } = paramsSchema.parse(params);

  const gameServers = await prisma.gameServer.findMany({
    where: {
      AND: [
        { lastSnapshot: { isNot: null } },
        {
          lastSnapshot: {
            map: {
              gameTypeName: { equals: gameTypeName },
            },
          },
        },
      ],
    },
    orderBy: [
      {
        lastSnapshot: {
          numClients: 'desc',
        },
      },
      {
        lastSnapshot: {
          maxClients: 'desc',
        },
      },
    ],
    include: {
      lastSnapshot: {
        include: {
          map: true,
        },
      },
    },
    take: 100,
    skip: (page - 1) * 100,
  });

  const servers = gameServers.reduce<
    ComponentProps<typeof ServerList>['servers']
  >((arr, gameServer) => {
    if (gameServer.lastSnapshot !== null) {
      arr.push({
        rank: (page - 1) * 100 + arr.length + 1,
        name: gameServer.lastSnapshot!.name,
        gameTypeName: gameServer.lastSnapshot!.map.gameTypeName,
        mapName: gameServer.lastSnapshot!.map.name ?? '',
        numClients: gameServer.lastSnapshot!.numClients,
        maxClients: gameServer.lastSnapshot!.maxClients,
        ip: gameServer.ip,
        port: gameServer.port,
      });
    }

    return arr;
  }, []);

  return <ServerList servers={servers} />;
}
