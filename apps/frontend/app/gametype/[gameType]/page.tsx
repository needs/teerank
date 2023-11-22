import { List, ListCell } from '@teerank/frontend/components/List';
import prisma from '@teerank/frontend/utils/prisma';
import { searchParamsSchema } from './schema';

export const metadata = {
  title: 'Players',
  description: 'List of ranked players',
};

export default async function Index({
  params,
  searchParams,
}: {
  params: { gameType: string };
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const parsedSearchParams = searchParamsSchema.parse(searchParams);

  const mapConditional = {
    name: { equals: parsedSearchParams.map ?? null },
  };

  const gameType = decodeURIComponent(params.gameType);

  const gameTypeConditional = {
    gameTypeName: { equals: gameType },
  };

  const playerInfos = await prisma.playerInfo.findMany({
    where: {
      map: {
        ...mapConditional,
        ...gameTypeConditional,
      },
    },
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
            href={{
              pathname: `/player/${encodeURIComponent(playerInfo.player.name)}`,
            }}
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
