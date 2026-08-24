import { Metadata } from 'next';
import { PlayerList } from '../../../../../components/PlayerList';
import { encodeString } from '../../../../../utils/encoding';
import prisma from '../../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from './schema';
import { notFound } from 'next/navigation';
import { z } from 'zod';
import { STUB_MIN_POLL_COUNT, STUB_OCCURRENCE_RATIO } from '@teerank/teerank';
import {
  countMapPlayersWithoutShared,
  listMapPlayersWithoutShared,
} from '@prisma/client/sql';
import { sharedHiddenParam } from '../../../../../utils/shared';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { gameTypeName, mapName } = paramsSchema.parse(params);

  return {
    title: `${mapName} - ${gameTypeName}`,
    description: `List of ranked players for ${mapName} in ${gameTypeName}`,
    alternates: {
      canonical: `https://teerank.io/gametype/${encodeString(gameTypeName)}/map/${encodeString(mapName)}`,
    },
  };
}

export default async function Index({
  params,
  searchParams,
}: {
  params: { [key: string]: string };
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamsSchema.parse(searchParams);
  const { gameTypeName, mapName } = paramsSchema.parse(params);

  let nameFilter: string[] | undefined;
  let filteredCount: number | undefined;

  if (sharedHiddenParam(searchParams)) {
    const [names, counts] = await Promise.all([
      prisma.$queryRawTyped(
        listMapPlayersWithoutShared(
          mapName,
          gameTypeName,
          STUB_MIN_POLL_COUNT,
          STUB_OCCURRENCE_RATIO,
          100,
          (page - 1) * 100
        )
      ),
      prisma.$queryRawTyped(
        countMapPlayersWithoutShared(
          mapName,
          gameTypeName,
          STUB_MIN_POLL_COUNT,
          STUB_OCCURRENCE_RATIO
        )
      ),
    ]);

    nameFilter = names.map((row) => row.playerName);
    filteredCount = Number(counts[0]?.count ?? 0);
  }

  const map = await prisma.map.findUnique({
    select: {
      gameType: {
        select: {
          rankMethod: true,
        },
      },
      playerCount: true,
      playerInfoMaps: {
        where:
          nameFilter === undefined
            ? undefined
            : { playerName: { in: nameFilter } },
        select: {
          rating: true,
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
                      gameServer: true,
                    },
                  },
                },
              },
            },
          },
          playTime: true,
        },
        orderBy: [
          {
            rating: {
              sort: 'desc',
              nulls: 'last',
            },
          },
          {
            playTime: 'desc',
          },
        ],
        take: 100,
        skip: nameFilter === undefined ? (page - 1) * 100 : 0,
      },
    },
    where: {
      name_gameTypeName: {
        name: mapName,
        gameTypeName: gameTypeName,
      },
    },
  });

  if (map === null) {
    return notFound();
  }

  return (
    <PlayerList
      playerCount={filteredCount ?? map.playerCount}
      rankMethod={map.gameType.rankMethod}
      players={map.playerInfoMaps.map((playerInfoMap, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: playerInfoMap.player.name,
        clan: playerInfoMap.player.clanName ?? undefined,
        rating: playerInfoMap.rating ?? undefined,
        playTime: playerInfoMap.playTime,
        lastSeenAt: playerInfoMap.player.lastSeenAt,
        pollCount: playerInfoMap.player.pollCount,
        occurrenceCount: playerInfoMap.player.occurrenceCount,
        gameServers: playerInfoMap.player.gameServerStateClients.map((client) => ({
          ip: client.gameServerState.gameServer?.ip ?? '',
          port: client.gameServerState.gameServer?.port ?? 0,
        })),
      }))}
    />
  );
}
