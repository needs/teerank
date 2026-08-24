import { PlayerList } from '../../components/PlayerList';
import {
  getGlobalCounts,
  STUB_MIN_POLL_COUNT,
  STUB_OCCURRENCE_RATIO,
} from '@teerank/teerank';
import {
  countPlayersWithoutShared,
  listPlayersWithoutShared,
} from '@prisma/client/sql';
import prisma from '../../utils/prisma';
import { searchParamSchema } from './schema';
import redis from '../../utils/redis';
import { sharedHiddenParam } from '../../utils/shared';

export const metadata = {
  title: 'All Players - Teerank',
  description: 'Teerank is a simple and fast ranking system for Teeworlds.',
  alternates: {
    canonical: 'https://teerank.io/all',
  },
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamSchema.parse(searchParams);

  let nameFilter: string[] | undefined;
  let filteredCount: number | undefined;

  if (sharedHiddenParam(searchParams)) {
    const [names, counts] = await Promise.all([
      prisma.$queryRawTyped(
        listPlayersWithoutShared(
          STUB_MIN_POLL_COUNT,
          STUB_OCCURRENCE_RATIO,
          100,
          (page - 1) * 100
        )
      ),
      prisma.$queryRawTyped(
        countPlayersWithoutShared(STUB_MIN_POLL_COUNT, STUB_OCCURRENCE_RATIO)
      ),
    ]);

    nameFilter = names.map((row) => row.name);
    filteredCount = Number(counts[0]?.count ?? 0);
  }

  const players = await prisma.player.findMany({
    where: nameFilter === undefined ? undefined : { name: { in: nameFilter } },
    select: {
      name: true,
      playTime: true,
      clanName: true,
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
    orderBy: {
      playTime: 'desc',
    },
    take: 100,
    skip: nameFilter === undefined ? (page - 1) * 100 : 0,
  });

  const globalCounts = await getGlobalCounts(redis);

  return (
    <>
      <p className="hidden">
        {`Teerank is a simple and fast ranking system for Teeworlds.`}
      </p>
      <PlayerList
        playerCount={filteredCount ?? globalCounts.players}
        rankMethod={null}
        showLastSeen={true}
        players={players.map((player, index) => ({
          rank: (page - 1) * 100 + index + 1,
          name: player.name,
          clan: player.clanName ?? undefined,
          rating: undefined,
          playTime: player.playTime,
          lastSeenAt: player.lastSeenAt,
          pollCount: player.pollCount,
          occurrenceCount: player.occurrenceCount,
          gameServers: player.gameServerStateClients.map((client) => ({
            ip: client.gameServerState.gameServer?.ip ?? '',
            port: client.gameServerState.gameServer?.port ?? 0,
          })),
        }))}
      />
    </>
  );
}
