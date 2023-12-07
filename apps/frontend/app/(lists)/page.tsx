import { PlayerList } from '../../components/PlayerList';
import prisma from '../../utils/prisma';
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
  const { page } = searchParamSchema.parse(searchParams);

  const players = await prisma.player.findMany({
    select: {
      name: true,
      playTime: true,
      clanName: true,

      gameServerClients: {
        select: {
          snapshot: {
            select: {
              createdAt: true,
              gameServerLast: {
                select: {
                  ip: true,
                  port: true,
                },
              },
            },
          },
        },

        orderBy: {
          snapshot: {
            createdAt: 'desc',
          },
        },

        take: 1,
      },
    },
    orderBy: {
      playTime: 'desc',
    },
    take: 100,
    skip: (page - 1) * 100,
  });

  const playerCount = await prisma.player.count();

  return (
    <PlayerList
      playerCount={playerCount}
      rankMethod={null}
      showLastSeen={true}
      players={players.map((player, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: player.name,
        clan: player.clanName ?? undefined,
        rating: undefined,
        playTime: player.playTime,
        lastSeen: {
          at: player.gameServerClients[0]?.snapshot.createdAt ?? new Date(),
          lastSnapshot: player.gameServerClients[0]?.snapshot.gameServerLast,
        },
      }))}
    />
  );
}
