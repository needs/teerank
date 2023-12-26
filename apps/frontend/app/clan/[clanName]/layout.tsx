import { notFound } from 'next/navigation';
import prisma from '../../../utils/prisma';
import { LayoutTabs } from './LayoutTabs';
import { paramsSchema } from './schema';
import { z } from 'zod';
import { formatPlayTime } from '../../../utils/format';

export default async function Index({
  params,
  children,
}: {
  params: z.infer<typeof paramsSchema>;
  children: React.ReactNode;
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
          playTime: true,
        },
      },
    },
    where: {
      name: clanName,
    },
  });

  const [gameTypeCount, mapCount] = await Promise.all([
    prisma.clanInfoGameType.count({
      where: {
        clan: {
          name: clanName,
        },
      }
    }),
    prisma.clanInfoMap.count({
      where: {
        clan: {
          name: clanName,
        },
      }
    }),
  ]);

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

      <LayoutTabs
        clanName={clanName}
        playerCount={clan.players.length}
        gameTypeCount={gameTypeCount}
        mapCount={mapCount}
      />

      {children}
    </main>
  );
}
