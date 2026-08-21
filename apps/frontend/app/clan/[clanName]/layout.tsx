import { notFound } from 'next/navigation';
import prisma from '../../../utils/prisma';
import { LayoutTabs } from './LayoutTabs';
import { paramsSchema } from './schema';
import { z } from 'zod';
import { formatPlayTime } from '../../../utils/format';
import { encodeString } from '../../../utils/encoding';
import { ClanPlayerCount } from './ClanPlayerCount';
import { ActivityHeader } from '../../../components/ActivityHeader';
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
        <ActivityHeader
          apiPath={`/api/clan/${encodeString(clanName)}/activity`}
          activity={activity}
          contentClassName="flex-col justify-center gap-2"
        >
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
        </ActivityHeader>
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
