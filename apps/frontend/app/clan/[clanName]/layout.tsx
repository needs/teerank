import { notFound } from 'next/navigation';
import prisma from '../../../utils/prisma';
import { LayoutTabs } from './LayoutTabs';
import { paramsSchema } from './schema';
import { z } from 'zod';
import { formatPlayTime } from '../../../utils/format';
import { encodeString } from '../../../utils/encoding';
import { ClanPlayerCount } from './ClanPlayerCount';
import { ActivityCalendarSection } from '../../../components/ActivityCalendarSection';
import { getClanActivity } from '../../../utils/activity';

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
      id: true,
      name: true,
      playTime: true,
      activePlayerCount: true,
      _count: {
        select: { clanPlayerInfos: true },
      },
    },
    where: {
      name: clanName,
    },
  });

  if (clan === null) {
    return notFound();
  }

  const [gameTypeCount, mapCount, activity] = await Promise.all([
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
    getClanActivity(clan.id, { range: '1y' }),
  ]);

  return (
    <main className="flex flex-col gap-8 py-12">
      <header className="px-8 xl:px-20">
        <div className="relative">
          <ActivityCalendarSection
            apiPath={`/api/clan/${encodeString(clanName)}/activity`}
            initial={activity}
          />
          <div className="absolute inset-y-0 left-0 z-10 flex w-1/2 flex-col justify-center gap-2 bg-gradient-to-r from-white from-30% via-white/85 via-65% to-transparent">
            <h1 className="text-2xl font-bold">{clan.name}</h1>
            <div className="flex flex-row divide-x">
              <span className="pr-4">
                <ClanPlayerCount
                  activeCount={clan.activePlayerCount}
                  totalCount={clan._count.clanPlayerInfos}
                />
              </span>
              <span className="px-4">
                Playtime: {formatPlayTime(clan.playTime)}
              </span>
            </div>
          </div>
        </div>
      </header>

      <LayoutTabs
        clanName={clanName}
        playerCount={clan.activePlayerCount}
        gameTypeCount={gameTypeCount}
        mapCount={mapCount}
      />

      {children}
    </main>
  );
}
