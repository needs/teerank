import { List, ListCell } from '@teerank/frontend/components/List';
import prisma from '@teerank/frontend/utils/prisma';
import { searchParamSchema } from './schema';
import { PlayerList } from '@teerank/frontend/components/PlayerList';

export const metadata = {
  title: 'Players',
  description: 'List of all players',
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamSchema.parse(searchParams);

  const players = await prisma.player.findMany({
    orderBy: {
      playTime: 'desc',
    },
    take: 100,
    skip: (page - 1) * 100,
  });

  return (
    <PlayerList
      hideRating={true}
      players={players.map((player, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: player.name,
        clan: undefined,
        rating: undefined,
        playTime: player.playTime,
      }))}
    />
  );
}
