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
  const parsedSearchParams = searchParamSchema.parse(searchParams);

  const gameServers = await prisma.gameServer.findMany({
    where: {
      lastSnapshot: {
        isNot: null,
      },
    },
    orderBy: {
      lastSnapshot: {
        numClients: 'desc',
      }
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
    skip: (parsedSearchParams.page - 1) * 100,
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
      {gameServers.map((gameServer, index) => gameServer.lastSnapshot !== null && (
        <>
          <ListCell alignRight label={`${index + 1}`} />
          <ListCell
            label={gameServer.lastSnapshot.name}
            href={{
              pathname: `/server/${encodeURIComponent(
                gameServer.ip
              )}/${encodeURIComponent(gameServer.port)}`,
            }}
          />
          <ListCell
            label={gameServer.lastSnapshot.map.gameTypeName}
            href={{
              pathname: `/gametype/${encodeURIComponent(
                gameServer.lastSnapshot.map.gameTypeName
              )}/servers`,
            }}
          />
          <ListCell
            label={gameServer.lastSnapshot.map.name ?? ''}
            href={{
              pathname: `/gametype/${encodeURIComponent(
                gameServer.lastSnapshot.map.gameTypeName
              )}/servers`,
              query: {
                map: gameServer.lastSnapshot.map.name,
              },
            }}
          />
          <ListCell
            alignRight
            label={`${gameServer.lastSnapshot.numClients} / ${gameServer.lastSnapshot.maxClients}`}
          />
        </>
      ))}
    </List>
  );
}
