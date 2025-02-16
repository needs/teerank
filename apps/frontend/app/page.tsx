import Link from 'next/link';
import { twMerge } from 'tailwind-merge';
import prisma from '../utils/prisma';
import { getGlobalCounts } from '@teerank/teerank';
import { formatInteger } from '../utils/format';
import { encodeString } from '../utils/encoding';
import redis from '../utils/redis';

export const metadata = {
  title: 'Teerank',
  description: 'Teerank is a simple and fast ranking system for Teeworlds.',
};

export const dynamic = 'force-dynamic';

function rankColor(rank: number) {
  switch (rank) {
    case 1:
      return 'from-[#F8D96E]';
    case 2:
      return 'from-[#CDCDCD]';
    case 3:
      return 'from-[#AB6E29]';
    default:
      return '';
  }
}

function RankBadge({ rank }: { rank: number }) {
  return (
    <span
      className={twMerge(
        'text-center py-2 bg-gradient-to-r from-[#F8D96E] to-transparent font-bold',
        rankColor(rank)
      )}
    >
      {rank}
    </span>
  );
}

function RankCard({
  title,
  titleHref,
  count,
  rankings,
  formatRankingHref,
}: {
  title: string;
  titleHref: string;
  count: number;
  rankings: string[];
  formatRankingHref: (ranking: string) => string;
}) {
  return (
    <section
      className="grid grid-cols-2 border rounded-xl overflow-clip bg-white shadow-md"
      style={{
        gridTemplateColumns: '4rem 1fr',
      }}
    >
      <span />
      <Link
        className="flex flex-row divide-x py-3 items-center justify-center col-span-2 group"
        href={titleHref}
      >
        <h3 className="text-lg font-bold text-center px-4 group-hover:underline">
          {title}
        </h3>
        <p className="px-4 text-[#999]">{formatInteger(count)}</p>
      </Link>
      {rankings.map((ranking, index) => (
        <>
          <RankBadge rank={index + 1} />
          <Link
            href={formatRankingHref(ranking)}
            className="px-4 py-2 hover:underline"
          >
            {ranking}
          </Link>
        </>
      ))}
    </section>
  );
}

export default async function Index() {
  const [
    players,
    clans,
    gameTypes,
    globalCounts,
  ] = await Promise.all([
    prisma.player.findMany({
      select: {
        name: true,
      },
      orderBy: {
        playTime: 'desc',
      },
      take: 3,
    }),

    prisma.clan.findMany({
      select: {
        name: true,
      },
      orderBy: [
        {
          playTime: 'desc',
        },
      ],
      take: 3,
    }),

    prisma.gameType.findMany({
      select: {
        name: true,
      },
      orderBy: [
        {
          playerCount: 'desc',
        },
        {
          mapCount: 'desc',
        },
      ],
      take: 3,
    }),

    getGlobalCounts(redis),
  ]);

  return (
    <>
      <p className="hidden">
        Teerank is a simple and fast ranking system for Teeworlds.
      </p>
      <header className="grid grid-cols-1 lg:grid-cols-3 gap-4 lg:gap-8 bg-gradient-to-b from-transparent to-[#CDCDCD] py-4 lg:py-12 px-4 md:px-12">
        <RankCard
          title="Players"
          titleHref="/all"
          count={globalCounts.players}
          rankings={players.map((player) => player.name)}
          formatRankingHref={(playerName) =>
            `/player/${encodeString(playerName)}`
          }
        />
        <RankCard
          title="Clans"
          titleHref="/all/clans"
          count={globalCounts.clans}
          rankings={clans.map((clan) => clan.name)}
          formatRankingHref={(clanName) => `/clan/${encodeString(clanName)}`}
        />
        <RankCard
          title="Gametypes"
          titleHref="/gametypes"
          count={globalCounts.gameTypes}
          rankings={gameTypes.map((gameType) => gameType.name)}
          formatRankingHref={(gameType) =>
            `/gametype/${encodeString(gameType)}`
          }
        />
      </header>

      <main className="py-12 px-4 md:px-12 xl:px-20 text-[#666] flex flex-col gap-8">
        <section className="flex flex-col gap-4">
          <header className="flex flex-row justify-between items-baseline">
            <h1 className="text-2xl font-bold clear-both">Faster search</h1>
            <p className="text-md clear-both text-[#888]">February 16th 2025</p>
          </header>
          <p>
            The search has been improved to be instant and more tolerant to
            typos.
          </p>
          <p>
            The search is now using a new database index and a new algorithm to
            find the best matches.  The database has been greatly cleaned up
            and optimized for this to not bring down the whole project.
          </p>
          <p>
            The effort to shrink database down, migrate to BullMQ, add more
            monitoring and improve performance is almost over, opening the way
            for upcoming features.
          </p>
        </section>

        <section className="flex flex-col gap-4">
          <header className="flex flex-row justify-between items-baseline">
            <h1 className="text-2xl font-bold clear-both">About the recent outages</h1>
            <p className="text-md clear-both text-[#888]">November 28th 2024</p>
          </header>
          <p>
            Teerank was down for a few hours on November 28th 2024 due to a
            database outage. The database has been restored and the data should
            be consistent again.
          </p>
          <p>
            An ongoing effort to shrink the database down and improve the
            performance is also being worked on. The current database is 110GB
            and growing as much every year. For a pet project, that&apos;s not
            sustainable.
          </p>
          <p>
            The plan is to move all server snapshots to a cheap object storage,
            and to use BullMQ + redis for the queue logic.
          </p>
          <p>
            All this performance work and database management is being done to
            prepare for upcoming features. More details will be announced soon.
          </p>
        </section>

        <section className="flex flex-col gap-4">
          <header className="flex flex-row justify-between items-baseline">
            <h1 className="text-2xl font-bold clear-both">New search!</h1>
            <p className="text-md clear-both text-[#888]">November 26th 2024</p>
          </header>
          <p>
            Search has been reworked to be faster and more tolerant to typos.
          </p>
        </section>

        <section className="flex flex-col gap-4">
          <header className="flex flex-row justify-between items-baseline">
            <h1 className="text-2xl font-bold clear-both">New home page!</h1>
            <p className="text-md clear-both text-[#888]">August 16th 2024</p>
          </header>
          <p>
            This very new home page will centralize news and main stats about
            Teerank and Teeworlds. List of all players is now available{' '}
            <Link href="/all" className="text-[#970] hover:underline">
              here
            </Link>{' '}
            or in the &quot;Rankings&quot; tab.
          </p>
        </section>
      </main>
    </>
  );
}
