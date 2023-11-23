import { List, ListCell } from '@teerank/frontend/components/List';
import prisma from '@teerank/frontend/utils/prisma';
import { searchParamsSchema } from '../schema';

export const metadata = {
  title: 'Maps',
  description: 'List of ranked maps',
};

export default async function Index({
  params,
  searchParams,
}: {
  params: { gameType: string };
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const parsedSearchParams = searchParamsSchema.parse(searchParams);
  const gameType = decodeURIComponent(params.gameType);

  const where =
    parsedSearchParams.map === undefined
      ? {
          gameType: {
            name: { equals: gameType },
          },
        }
      : {
          map: {
            name: { equals: parsedSearchParams.map },
          },
        };

  const playerInfos = await prisma.playerInfo.findMany({
    where,
    orderBy: [
      {
        rating: 'desc',
      },
      {
        playTime: 'desc',
      },
    ],
    include: {
      player: true,
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
          title: 'Elo',
          expand: false,
        },
        {
          title: 'Last Seen',
          expand: false,
        },
      ]}
    >
      {playerInfos.map((playerInfo, index) => (
        <>
          <ListCell alignRight label={`${index + 1}`} />
          <ListCell
            label={playerInfo.player.name}
            href={{ pathname: `/players/${playerInfo.player.name}` }}
          />
          <ListCell label={''} />
          <ListCell
            alignRight
            label={
              playerInfo.rating === null
                ? ''
                : `${Intl.NumberFormat('en-US', {
                    maximumFractionDigits: 0,
                  }).format(playerInfo.rating)}`
            }
          />
          <ListCell alignRight label="1 hour ago" />
        </>
      ))}
    </List>
  );
}
