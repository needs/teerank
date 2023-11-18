import { List, ListCell } from '@teerank/frontend/components/List';
import prisma from '@teerank/frontend/utils/prisma';
import { searchParamSchema } from '../(lists)/schema';

export const metadata = {
  title: 'Gametypes',
  description: 'List of all gametypes',
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const parsedSearchParams = searchParamSchema.parse(searchParams);

  const maps = await prisma.map.groupBy({
    by: ['gameTypeName'],
    orderBy: {
      _count: {
        gameTypeName: 'desc',
      },
    },
    _count: {
      gameTypeName: true,
    },
    take: 100,
    skip: (parsedSearchParams.page - 1) * 100,
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
            title: 'Maps',
            expand: false,
          },
        ]}
      >
        {maps.map((map, index) => (
          <>
            <ListCell alignRight label={`${index + 1}`} />
            <ListCell
              label={map.gameTypeName}
              href={{
                pathname: `/gametype/${encodeURIComponent(map.gameTypeName)}`,
              }}
            />
            <ListCell label={map._count.gameTypeName.toString()} />
          </>
        ))}
      </List>
    </div>
  );
}
