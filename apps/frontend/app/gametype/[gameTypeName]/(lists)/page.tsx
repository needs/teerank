import { RankMethod } from '@prisma/client';
import { PlayerList } from '../../../../components/PlayerList';
import prisma from '../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from '../schema';
import { notFound } from 'next/navigation';
import { encodeString } from '../../../../utils/encoding';
import { Metadata } from 'next';
import { z } from 'zod';
import { STUB_MIN_POLL_COUNT, STUB_OCCURRENCE_RATIO } from '@teerank/teerank';
import {
  countGameTypePlayersWithoutShared,
  listGameTypePlayersWithoutShared,
} from '@prisma/client/sql';
import { sharedHiddenParam } from '../../../../utils/shared';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { gameTypeName } = paramsSchema.parse(params);

  return {
    title: `Gametype ${gameTypeName}`,
    description: `List of ranked players for ${gameTypeName}`,
    alternates: {
      canonical: `https://teerank.io/gametype/${encodeString(gameTypeName)}`,
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
  const { gameTypeName } = paramsSchema.parse(params);

  let nameFilter: string[] | undefined;
  let filteredCount: number | undefined;

  if (sharedHiddenParam(searchParams)) {
    const [names, counts] = await Promise.all([
      prisma.$queryRawTyped(
        listGameTypePlayersWithoutShared(
          gameTypeName,
          STUB_MIN_POLL_COUNT,
          STUB_OCCURRENCE_RATIO,
          100,
          (page - 1) * 100
        )
      ),
      prisma.$queryRawTyped(
        countGameTypePlayersWithoutShared(
          gameTypeName,
          STUB_MIN_POLL_COUNT,
          STUB_OCCURRENCE_RATIO
        )
      ),
    ]);

    nameFilter = names.map((row) => row.playerName);
    filteredCount = Number(counts[0]?.count ?? 0);
  }

  const gameType = await prisma.gameType.findUnique({
    select: {
      rankMethod: true,
      playerCount: true,
      playerInfoGameTypes: {
        where:
          nameFilter === undefined
            ? undefined
            : { playerName: { in: nameFilter } },
        select: {
          rating: true,
          playTime: true,
          playerName: true,
          player: {
            select: {
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
        },
        orderBy: [
          {
            rating: {
              sort: 'desc',
              nulls: 'last',
            }
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
      name: gameTypeName,
    },
  });

  if (gameType === null) {
    return notFound();
  }

  return (
    <PlayerList
      playerCount={filteredCount ?? gameType.playerCount}
      rankMethod={gameType.rankMethod === RankMethod.TIME ? null : gameType.rankMethod}
      players={gameType.playerInfoGameTypes.map((playerInfoGameType, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: playerInfoGameType.playerName,
        clan: playerInfoGameType.player.clanName ?? undefined,
        rating: playerInfoGameType.rating ?? undefined,
        playTime: playerInfoGameType.playTime,
        lastSeenAt: playerInfoGameType.player.lastSeenAt,
        pollCount: playerInfoGameType.player.pollCount,
        occurrenceCount: playerInfoGameType.player.occurrenceCount,
        gameServers: playerInfoGameType.player.gameServerStateClients.map((client) => ({
          ip: client.gameServerState.gameServer?.ip ?? '',
          port: client.gameServerState.gameServer?.port ?? 0,
        })),
      }))}
    />
  );
}
