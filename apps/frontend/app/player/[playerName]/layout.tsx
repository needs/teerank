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
import { ActivityHeader } from '../../../components/ActivityHeader';
import { SharedRatio } from '../../../components/SharedRatio';
import { getPlayerActivity } from '../../../utils/activity';
import { countTeammates } from '../../../utils/teammates';

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
      pollCount: true,
      occurrenceCount: true,

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

  const [clanCount, teammateCount, activity] = await Promise.all([
    prisma.clanPlayerInfo.count({
      where: {
        playerName,
      },
    }),
    countTeammates(player.id),
    getPlayerActivity(player.id, { range: '1y' }),
  ]);

  return (
    <main className="flex flex-col gap-8 py-12">
      <header className="px-8 xl:px-20">
        <ActivityHeader
          apiPath={`/api/player/${encodeString(playerName)}/activity`}
          activity={activity}
          contentClassName="flex-row items-center gap-4"
        >
          <Image src="/player.png" width={100} height={100} alt="Player" />
          <section className="flex flex-col gap-2">
            <div className="flex flex-row items-center gap-3">
              <h1 className="text-2xl font-bold">{player.name}</h1>
              <SharedRatio
                pollCount={player.pollCount}
                occurrenceCount={player.occurrenceCount}
              />
            </div>
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
                  playerName={player.name}
                />
              </span>
            </div>
          </section>
        </ActivityHeader>
      </header>

      <LayoutTabs
        playerName={playerName}
        clanCount={clanCount}
        teammateCount={teammateCount}
      />

      {children}
    </main>
  );
}
