import { List, ListCell } from '@teerank/frontend/components/List';
import prisma from '@teerank/frontend/utils/prisma';
import { searchParamSchema } from '../(lists)/schema';
import { formatInteger } from '@teerank/frontend/utils/format';

export const metadata = {
  title: 'Gametypes',
  description: 'List of all gametypes',
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamSchema.parse(searchParams);

  const gametypes = await prisma.gameType.findMany({
    select: {
      name: true,
      _count: {
        select: {
          playerInfos: true,
          map: true,
          clanInfos: true,
        },
      },
    },
    orderBy: {
      playerInfos: {
        _count: 'desc',
      },
    },
    take: 100,
    skip: (page - 1) * 100,
  });

  return (
    <div className="py-8">
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
            title: 'Players',
            expand: false,
          },
          {
            title: 'Clans',
            expand: false,
          },
          {
            title: 'Maps',
            expand: false,
          },
        ]}
      >
        {gametypes.map((gametype, index) => (
          <>
            <ListCell
              alignRight
              label={formatInteger((page - 1) * 100 + index + 1)}
            />
            <ListCell
              label={gametype.name}
              href={{
                pathname: `/gametype/${encodeURIComponent(gametype.name)}`,
              }}
            />
            <ListCell alignRight label={formatInteger(gametype._count.playerInfos)} />
            <ListCell alignRight label={formatInteger(gametype._count.clanInfos)} />
            <ListCell alignRight label={formatInteger(gametype._count.map)} />
          </>
        ))}
      </List>
    </div>
  );
}
