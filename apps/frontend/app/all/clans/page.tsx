import { ClanList } from '../../../components/ClanList';
import { getGlobalCounts } from '@teerank/teerank';
import prisma from '../../../utils/prisma';
import redis from '../../../utils/redis';
import { searchParamSchema } from '../schema';

export const metadata = {
  title: 'All Clans - Teerank',
  description: 'Teerank is a simple and fast ranking system for Teeworlds.',
  alternates: {
    canonical: 'https://teerank.io/all/clans',
  },
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamSchema.parse(searchParams);

  const [clans, globalCounts] = await Promise.all([
    prisma.clan.findMany({
      select: {
        name: true,
        playTime: true,
        activePlayerCount: true,
      },
      orderBy: [
        {
          playTime: 'desc',
        },
      ],
      take: 100,
      skip: (page - 1) * 100,
    }),

    getGlobalCounts(redis),
  ]);

  return (
    <ClanList
      clanCount={globalCounts.clans}
      clans={clans.map((clan, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: clan.name,
        playerCount: clan.activePlayerCount,
        playTime: clan.playTime,
      }))}
    />
  );
}
