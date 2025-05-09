import { Metadata } from 'next';
import { ServerList } from '../../../../../../components/ServerList';
import prisma from '../../../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from '../schema';
import { encodeString } from '../../../../../../utils/encoding';
import { z } from 'zod';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { gameTypeName, mapName } = paramsSchema.parse(params);

  return {
    title: `${mapName} - ${gameTypeName}`,
    description: `List of ranked servers for ${mapName} in ${gameTypeName}`,
    alternates: {
      canonical: `https://teerank.io/gametype/${encodeString(gameTypeName)}/map/${encodeString(mapName)}/servers`,
    },
  };
}

export default async function Index({
  params,
  searchParams,
}: {
  params: { [key: string]: string };
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamsSchema.parse(searchParams);
  const { gameTypeName, mapName } = paramsSchema.parse(params);

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
              name: true,
              gameTypeName: true,
            },
          },
        },
      },
    },
    where: {
      gameServerState: {
        map: {
          name: {
            equals: mapName,
          },
          gameTypeName: {
            equals: gameTypeName,
          },
        },
      },
    },
    orderBy: [
      {
        gameServerState: {
          numClients: 'desc',
        },
      },
      {
        gameServerState: {
          maxClients: 'desc',
        },
      },
    ],
    take: 100,
    skip: (page - 1) * 100,
  });

  const serverCount = await prisma.gameServer.count({
    where: {
      gameServerState: {
        map: {
          name: {
            equals: mapName,
          },
          gameTypeName: {
            equals: gameTypeName,
          },
        },
      },
    },
  });

  return (
    <ServerList
      serverCount={serverCount}
      servers={gameServers.map((gameServer, index) => ({
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
      }))}
    />
  );
}
