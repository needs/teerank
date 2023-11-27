import prisma from '@teerank/frontend/utils/prisma';
import { paramsSchema, searchParamsSchema } from '../schema';
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
  const { gameTypeName, mapName } = paramsSchema.parse(params);

  const snapshots = await prisma.gameServerSnapshot.findMany({
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
      gameServerLast: {
        select: {
          ip: true,
          port: true,
        },
      },
    },
    where: {
      gameServerLast: {
        isNot: null,
      },
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

  const serverCount = await prisma.gameServerSnapshot.count({
    where: {
      gameServerLast: {
        isNot: null,
      },
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
      servers={snapshots.map((snapshot, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: snapshot.name,
        gameTypeName: snapshot.map.gameTypeName,
        mapName: snapshot.map.name ?? '',
        numClients: snapshot.numClients,
        maxClients: snapshot.maxClients,
        ip: snapshot.gameServerLast!.ip,
        port: snapshot.gameServerLast!.port,
      }))}
    />
  );
}
