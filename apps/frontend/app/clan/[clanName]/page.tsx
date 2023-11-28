import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import prisma from '../../../utils/prisma';
import { PlayerList } from '../../../components/PlayerList';
import { formatPlayTime } from '../../../utils/format';

export const metadata = {
  title: 'Clan',
  description: 'A Teeworlds clan',
};

export default async function Index({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}) {
  const { clanName } = paramsSchema.parse(params);

  const clan = await prisma.clan.findUnique({
    select: {
      name: true,
      playTime: true,
      _count: {
        select: {
          players: true,
        },
      },
      players: {
        select: {
          name: true,
          playTime: true,
        },
        orderBy: {
          playTime: 'desc',
        },
      },
    },
    where: {
      name: clanName,
    },
  });

  if (clan === null) {
    return notFound();
  }

  return (
    <main className="flex flex-col gap-8 py-12">
      <header className="flex flex-row px-20 gap-4 items-center">
        <section className="flex flex-col gap-2 grow">
          <h1 className="text-2xl font-bold">{clan.name}</h1>
          <span className="pr-4">{`${clan.players.length} players`}</span>
        </section>
        <aside className="flex flex-col gap-2 text-right">
          <p>
            <b>Total Playtime</b>: {formatPlayTime(clan.playTime)}
          </p>
          <p>
            <i>Player Playtime</i>:{' '}
            {formatPlayTime(
              clan.players.reduce((acc, player) => acc + player.playTime, 0)
            )}
          </p>
        </aside>
      </header>

      <PlayerList
        playerCount={clan._count.players}
        hideRating={true}
        players={clan.players.map((player, index) => ({
          rank: index + 1,
          name: player.name,
          clan: clanName,
          playTime: player.playTime,
        }))}
      />
    </main>
  );
}
