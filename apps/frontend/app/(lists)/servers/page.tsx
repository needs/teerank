import { List, ListCell } from '@teerank/frontend/components/List';
import prisma from '@teerank/frontend/utils/prisma';
import { searchParamSchema } from '../schema';

export const metadata = {
  title: 'Servers',
  description: 'List of all servers',
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const validatedSearchParam = searchParamSchema.parse(searchParams);

  const gameServers = await prisma.gameServer.findMany({
    where: {
      snapshots: {
        some: {},
      },
    },
    orderBy: {
      createdAt: 'desc',
    },
    include: {
      snapshots: {
        orderBy: {
          createdAt: 'asc',
        },
        take: 1,
        include: {
          clients: true,
          map: true,
        },
      },
    },
    take: 100,
    skip: (validatedSearchParam.page - 1) * 100,
  });

  return (
    <List
      columns={[
        {
          title: '',
          expand: false,
        },
        {
          title: 'Name',
          expand: true,
        },
        {
          title: 'Game Type',
          expand: false,
        },
        {
          title: 'Map',
          expand: false,
        },
        {
          title: 'Players',
          expand: false,
        },
      ]}
    >
      {gameServers.map((gameServer, index) => (
        <>
          <ListCell alignRight label={`${index + 1}`} />
          <ListCell label={gameServer.snapshots[0].name} />
          <ListCell
            label={gameServer.snapshots[0].map.gameTypeName}
            href={{
              pathname: `/gametype/${encodeURIComponent(
                gameServer.snapshots[0].map.gameTypeName
              )}`,
            }}
          />
          <ListCell
            label={gameServer.snapshots[0].map.name ?? ''}
            href={{
              pathname: `/gametype/${encodeURIComponent(
                gameServer.snapshots[0].map.gameTypeName
              )}`,
              query: {
                map: gameServer.snapshots[0].map.name,
              },
            }}
          />
          <ListCell
            alignRight
            label={`${gameServer.snapshots[0].numClients} / ${gameServer.snapshots[0].maxClients}`}
          />
        </>
      ))}
    </List>
  );
}
