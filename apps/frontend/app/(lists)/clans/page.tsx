import { ClanList } from '../../../components/ClanList';
import { getGlobalCounts } from '../../../utils/globalCounts';
import prisma from '../../../utils/prisma';
import { searchParamSchema } from '../schema';

export const metadata = {
  title: 'Clans',
  description: 'List of all clans',
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamSchema.parse(searchParams);

  const clans = await prisma.clan.findMany({
    select: {
      name: true,
      playTime: true,
      _count: {
        select: {
          players: true,
        },
      },
    },
    orderBy: [
      {
        playTime: 'desc',
      },
      {
        players: {
          _count: 'desc',
        },
      },
    ],
    take: 100,
    skip: (page - 1) * 100,
  });

  const { clanCount } = await getGlobalCounts();

  return (
    <ClanList
      clanCount={clanCount}
      clans={clans.map((clan, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: clan.name,
        playerCount: clan._count.players,
        playTime: clan.playTime,
      }))}
    />
  );
}
