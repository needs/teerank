import { List, ListCell } from '@teerank/frontend/components/List';
import prisma from '@teerank/frontend/utils/prisma';
import { searchParamSchema } from './schema';

export const metadata = {
  title: 'Players',
  description: 'List of all players',
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const parsedSearchParams = searchParamSchema.parse(searchParams);

  const players = await prisma.player.findMany({
    orderBy: {
      createdAt: 'desc',
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
          title: 'Clan',
          expand: true,
        },
        {
          title: 'Created at',
          expand: false,
        },
      ]}
    >
      {players.map((player, index) => (
        <>
          <ListCell alignRight label={`${index + 1}`} />
          <ListCell label={player.name} href={{ pathname: `/player/${encodeURIComponent(player.name)}`}} />
          <ListCell label={''} />
          <ListCell alignRight label="1 hour ago" />
        </>
      ))}
    </List>
  );
}
