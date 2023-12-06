import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import Link from 'next/link';
import Image from 'next/image';
import { formatPlayTime } from '../../../utils/format';
import prisma from '../../../utils/prisma';
import { LayoutTabs } from './LayoutTabs';
import { LastSeen } from '../../../components/LastSeen';

export default async function Index({
  params,
  children,
}: {
  params: z.infer<typeof paramsSchema>;
  children: React.ReactNode;
}) {
  const { playerName } = paramsSchema.parse(params);

  const player = await prisma.player.findUnique({
    select: {
      name: true,
      playTime: true,
      clanName: true,

      playerInfos: {
        select: {
          map: {
            select: {
              name: true,
              gameTypeName: true,
            },
          },
          playTime: true,
        },
        where: {
          map: {
            isNot: null,
          },
        },
        orderBy: {
          playTime: 'desc',
        },
      },
    },
    where: {
      name: playerName,
    },
  });

  const counts = await prisma.$transaction([
    prisma.playerInfo.count({
      where: {
        playerName,
        map: {
          isNot: null,
        },
      },
    }),
    prisma.playerInfo.count({
      where: {
        playerName,
        gameType: {
          isNot: null,
        },
      },
    }),
    prisma.clanPlayerInfo.count({
      where: {
        playerName,
      },
    }),
  ]);

  const lastSnapshot = await prisma.gameServerSnapshot.findFirst({
    select: {
      createdAt: true,

      gameServerLast: {
        select: {
          ip: true,
          port: true,
        },
      },
    },
    where: {
      clients: {
        some: {
          playerName,
        },
      },
    },
    orderBy: {
      createdAt: 'desc',
    },
  });

  if (player === null) {
    return notFound();
  }

  return (
    <main className="flex flex-col gap-8 py-12">
      <header className="flex flex-row px-20 gap-4 items-center">
        <Image src="/player.png" width={100} height={100} alt="Player" />
        <section className="flex flex-col gap-2 grow">
          <h1 className="text-2xl font-bold">{player.name}</h1>
          <span className="pr-4">
            {player.clanName !== null && (
              <Link
                className="hover:underline"
                href={{
                  pathname: `/clan/${encodeURIComponent(player.clanName)}`,
                }}
              >
                {player.clanName}
              </Link>
            )}
          </span>
        </section>
        <aside className="flex flex-col gap-2 text-right">
          <p>
            <b>Playtime</b>: {formatPlayTime(player.playTime)}
          </p>
          {lastSnapshot !== null && (
            <LastSeen
              currentServer={lastSnapshot.gameServerLast}
              lastSeen={lastSnapshot.createdAt}
            />
          )}
        </aside>
      </header>

      <LayoutTabs
        playerName={playerName}
        mapCount={counts[0]}
        gameTypeCount={counts[1]}
        clanCount={counts[2]}
      />

      {children}
    </main>
  );
}
