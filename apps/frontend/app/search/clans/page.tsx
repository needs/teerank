import { searchParamSchema } from '../schema';
import { LayoutTabs } from '../LayoutTabs';
import uniqBy from 'lodash.uniqby';
import { Error } from '../SearchError';
import { ClanList } from '../../../components/ClanList';
import prisma from '../../../utils/prisma';

export const metadata = {
  title: 'Search - Clans',
  description: 'Search for clans',
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { query } = searchParamSchema.parse(searchParams);

  if (query.length < 2) {
    return <Error message="Please enter at least 2 characters." />;
  }

  const matches = await Promise.all([
    prisma.clan.findMany({
      where: {
        name: {
          equals: query,
          mode: 'insensitive',
        },
      },
      select: {
        name: true,
        playTime: true,
        _count: {
          select: {
            players: true,
          },
        },
      },
      orderBy: {
        name: 'asc',
      },
      take: 100,
    }),
    prisma.clan.findMany({
      where: {
        OR: [
          {
            name: {
              startsWith: query,
              mode: 'insensitive',
            },
          },
          {
            name: {
              endsWith: query,
              mode: 'insensitive',
            },
          },
        ],
      },
      select: {
        name: true,
        playTime: true,
        _count: {
          select: {
            players: true,
          },
        },
      },
      orderBy: {
        name: 'asc',
      },
      take: 100,
    }),
    prisma.clan.findMany({
      where: {
        name: {
          contains: query,
          mode: 'insensitive',
        },
      },
      select: {
        name: true,
        playTime: true,
        _count: {
          select: {
            players: true,
          },
        },
      },
      orderBy: {
        name: 'asc',
      },
      take: 100,
    }),
  ]);

  const clans = uniqBy(matches.flat(), 'name').slice(0, 100);

  return (
    <LayoutTabs query={query} selectedTab="clans">
      {({ clanCount }) => (
        <ClanList
          clanCount={clanCount}
          clans={clans.map((clan, index) => ({
            rank: index + 1,
            name: clan.name,
            playerCount: clan._count.players,
            playTime: clan.playTime,
          }))}
        />
      )}
    </LayoutTabs>
  );
}
