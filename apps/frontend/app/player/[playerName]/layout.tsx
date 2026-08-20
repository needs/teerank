import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import Link from 'next/link';
import Image from 'next/image';
import prisma from '../../../utils/prisma';
import { LayoutTabs } from './LayoutTabs';
import { LastSeen } from '../../../components/LastSeen';
import { formatPlayTime } from '../../../utils/format';
import { encodeString } from '../../../utils/encoding';
import { ActivityCalendarSection } from '../../../components/ActivityCalendarSection';
import { getPlayerActivity } from '../../../utils/activity';

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
      id: true,
      name: true,
      clanName: true,
      playTime: true,
      lastSeenAt: true,

      gameServerStateClients: {
        select: {
          gameServerState: {
            select: {
              gameServer: {
                select: {
                  ip: true,
                  port: true,
                },
              },
            },
          },
        },
      },
    },
    where: {
      name: playerName,
    },
  });

  if (player === null) {
    return notFound();
  }

  const [mapCount, gameTypeCount, clanCount, activity] = await Promise.all([
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
    getPlayerActivity(player.id, { range: '1y' }),
  ]);

  return (
    <main className="flex flex-col gap-8 py-12">
      <header className="px-8 xl:px-20">
        <div className="relative">
          <ActivityCalendarSection
            apiPath={`/api/player/${encodeString(playerName)}/activity`}
            initial={activity}
          />
          <div className="absolute inset-y-0 left-0 z-10 flex w-1/2 flex-row items-center gap-4 bg-gradient-to-r from-white from-30% via-white/85 via-65% to-transparent">
            <Image src="/player.png" width={100} height={100} alt="Player" />
            <section className="flex flex-col gap-2">
              <h1 className="text-2xl font-bold">{player.name}</h1>
              <div className="flex flex-row divide-x">
                {player.clanName !== null && (
                  <span className="pr-4">
                    <Link
                      className="hover:underline"
                      href={{
                        pathname: `/clan/${encodeString(player.clanName)}`,
                      }}
                    >
                      {player.clanName}
                    </Link>
                  </span>
                )}
                <span className={player.clanName !== null ? 'px-4' : 'pr-4'}>
                  Playtime: {formatPlayTime(player.playTime)}
                </span>
                <span className="px-4">
                  <LastSeen
                    gameServers={player.gameServerStateClients
                      .map((client) => client.gameServerState.gameServer)
                      .filter((gameServer) => gameServer !== null)}
                    lastSeenAt={player.lastSeenAt}
                  />
                </span>
              </div>
            </section>
          </div>
        </div>
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
