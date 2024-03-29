import { PlayerList } from '../../components/PlayerList';
import { getGlobalCounts } from '../../utils/globalCounts';
import prisma from '../../utils/prisma';
import { searchParamSchema } from './schema';

export const metadata = {
  title: 'All Players - Teerank',
  description: 'Teerank is a simple and fast ranking system for Teeworlds.',
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

      lastGameServerClient: {
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
      }
    },
    orderBy: {
      playTime: 'desc',
    },
    take: 100,
    skip: (page - 1) * 100,
  });

  const { playerCount } = await getGlobalCounts();

  return (
    <>
    <p className="hidden">
      {`Teerank is a simple and fast ranking system for Teeworlds.`}
    </p>
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
        lastSeen: player.lastGameServerClient === null ? undefined : {
          at: player.lastGameServerClient.snapshot.createdAt,
          lastSnapshot: player.lastGameServerClient.snapshot.gameServerLast,
        },
      }))}
    />
    </>
  );
}
