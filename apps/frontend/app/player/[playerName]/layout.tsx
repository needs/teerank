import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import Link from 'next/link';
import Image from 'next/image';
import { formatPlayTime } from '../../../utils/format';
import prisma from '../../../utils/prisma';
import { LayoutTabs } from './LayoutTabs';
import { LastSeen } from '../../../components/LastSeen';
import { encodeString } from '../../../utils/encoding';

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
      },

      playerInfoMaps: {
        select: {
          map: {
            select: {
              name: true,
              gameTypeName: true,
            },
          },
          playTime: true,
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

  const [mapCount, gameTypeCount, clanCount] = await Promise.all([
    prisma.playerInfoMap.count({
      where: {
        playerName,
      },
    }),
    prisma.playerInfoGameType.count({
      where: {
        playerName,
      },
    }),
    prisma.clanPlayerInfo.count({
      where: {
        playerName,
      },
    }),
  ]);

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
                  pathname: `/clan/${encodeString(player.clanName)}`,
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
          <LastSeen
            lastSnapshot={
              player.lastGameServerClient?.snapshot.gameServerLast ?? null
            }
            lastSeen={
              player.lastGameServerClient?.snapshot.createdAt ?? new Date()
            }
          />
        </aside>
      </header>

      <LayoutTabs
        playerName={playerName}
        mapCount={mapCount}
        gameTypeCount={gameTypeCount}
        clanCount={clanCount}
      />

      {children}
    </main>
  );
}
