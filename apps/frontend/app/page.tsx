import Link from 'next/link';
import { twMerge } from 'tailwind-merge';
import prisma from '../utils/prisma';
import { getGlobalCounts } from '../utils/globalCounts';
import { formatInteger } from '../utils/format';
import { encodeString } from '../utils/encoding';

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
      <Link className="flex flex-row divide-x py-3 items-center justify-center col-span-2 group" href={titleHref}>
        <h3 className="text-lg font-bold text-center px-4 group-hover:underline">{title}</h3>
        <p className="px-4 text-[#999]">{formatInteger(count)}</p>
      </Link>
      {rankings.map((ranking, index) => (
        <>
          <RankBadge rank={index + 1} />
          <Link href={formatRankingHref(ranking)} className="px-4 py-2 hover:underline">
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
    gameTypesCount,
    { playerCount, clanCount },
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

    prisma.gameType.count(),
    getGlobalCounts(),
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
          count={playerCount}
          rankings={players.map((player) => player.name)}
          formatRankingHref={(playerName) => `/player/${encodeString(playerName)}`}
        />
        <RankCard
          title="Clans"
          titleHref="/all/clans"
          count={clanCount}
          rankings={clans.map((clan) => clan.name)}
          formatRankingHref={(clanName) => `/clan/${encodeString(clanName)}`}
        />
        <RankCard
          title="Gametypes"
          titleHref="/gametypes"
          count={gameTypesCount}
          rankings={gameTypes.map((gameTypes) => gameTypes.name)}
          formatRankingHref={(gameType) => `/gametype/${encodeString(gameType)}`}
        />
      </header>

      <main className="py-12 px-4 md:px-12 xl:px-20 text-[#666] flex flex-col gap-8">
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

        <section className="flex flex-col gap-4">
          <header className="flex flex-row justify-between items-baseline">
            <h1 className="text-2xl font-bold clear-both">New search!</h1>
            <p className="text-md clear-both text-[#888]">November 26th 2024</p>
          </header>
          <p>
            Search has been reworked to be faster and more tolerant to typos.
          </p>
        </section>
      </main>
    </>
  );
}
