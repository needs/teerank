import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound, redirect } from 'next/navigation';
import prisma from '../../../utils/prisma';
import { PlayerList } from '../../../components/PlayerList';
import { searchParamPageSchema } from '../../../utils/page';
import { Metadata } from 'next';
import { encodeString } from '../../../utils/encoding';
import { STUB_MIN_POLL_COUNT, STUB_OCCURRENCE_RATIO } from '@teerank/teerank';
import {
  countClanPlayersWithoutShared,
  listClanPlayersWithoutShared,
} from '@prisma/client/sql';
import { sharedHiddenParam } from '../../../utils/shared';
export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { clanName } = paramsSchema.parse(params);

  return {
    title: `Clan ${clanName}`,
    description: `List of ranked players for ${clanName}`,
    alternates: {
      canonical: `https://teerank.io/clan/${encodeString(clanName)}`,
    },
  };
}

export default async function Index({
  params,
  searchParams,
}: {
  params: z.infer<typeof paramsSchema>;
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { clanName } = paramsSchema.parse(params);
  const { page } = searchParamPageSchema.parse(searchParams);
  const showPastMembers = searchParams.past === 'true' || searchParams.past === '1';

  let nameFilter: string[] | undefined;
  let filteredCount: number | undefined;

  if (sharedHiddenParam(searchParams)) {
    const [names, counts] = await Promise.all([
      prisma.$queryRawTyped(
        listClanPlayersWithoutShared(
          clanName,
          showPastMembers,
          STUB_MIN_POLL_COUNT,
          STUB_OCCURRENCE_RATIO,
          100,
          (page - 1) * 100
        )
      ),
      prisma.$queryRawTyped(
        countClanPlayersWithoutShared(
          clanName,
          showPastMembers,
          STUB_MIN_POLL_COUNT,
          STUB_OCCURRENCE_RATIO
        )
      ),
    ]);

    nameFilter = names.map((row) => row.playerName);
    filteredCount = Number(counts[0]?.count ?? 0);
  }

  const clan = await prisma.clan.findUnique({
    select: {
      activePlayerCount: true,
      _count: {
        select: {
          clanPlayerInfos: true,
        },
      },
      clanPlayerInfos: {
        select: {
          playTime: true,
          player: {
            select: {
              name: true,
              clanName: true,
              lastSeenAt: true,
              pollCount: true,
              occurrenceCount: true,
              gameServerStateClients: {
                select: {
                  gameServerState: {
                    select: {
                      createdAt: true,
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
          },
        },
        where:
          nameFilter === undefined
            ? showPastMembers
              ? undefined
              : { player: { clanName } }
            : { playerName: { in: nameFilter } },
        orderBy: {
          playTime: 'desc',
        },
        take: 100,
        skip: nameFilter === undefined ? (page - 1) * 100 : 0,
      },
    },
    where: {
      name: clanName,
    },
  });

  if (clan === null) {
    return notFound();
  }

  const playerCount =
    filteredCount ??
    (showPastMembers ? clan._count.clanPlayerInfos : clan.activePlayerCount);
  const maxPage = Math.ceil(playerCount / 100) || 1;

  if (page > maxPage) {
    // Generate new URL with the max possible page
    const params = new URLSearchParams();
    for (const [key, value] of Object.entries(searchParams)) {
      if (value !== undefined) {
        if (Array.isArray(value)) {
          value.forEach((v) => params.append(key, v));
        } else {
          params.set(key, value);
        }
      }
    }
    params.set('page', maxPage.toString());
    return redirect(`/clan/${encodeURIComponent(clanName)}?${params.toString()}`);
  }

  return (
    <div className="flex flex-col gap-4">
      <PlayerList
        playerCount={playerCount}
        rankMethod={null}
        showLastSeen={true}
        showInactiveToggle={true}
        players={clan.clanPlayerInfos.map((playerInfo, index) => ({
          rank: index + 1,
          name: playerInfo.player.name,
          clan: playerInfo.player.clanName ?? undefined,
          isActiveClan: playerInfo.player.clanName === clanName,
          playTime: playerInfo.playTime,
          lastSeenAt: playerInfo.player.lastSeenAt,
          pollCount: playerInfo.player.pollCount,
          occurrenceCount: playerInfo.player.occurrenceCount,
          gameServers: playerInfo.player.gameServerStateClients.map((client) => ({
            ip: client.gameServerState.gameServer?.ip ?? '',
            port: client.gameServerState.gameServer?.port ?? 0,
          })),
        }))}
      />
    </div>
  );
}
