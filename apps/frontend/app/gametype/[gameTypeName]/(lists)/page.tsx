import { RankMethod } from '@prisma/client';
import { PlayerList } from '../../../../components/PlayerList';
import prisma from '../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from '../schema';
import { notFound } from 'next/navigation';
import { encodeString } from '../../../../utils/encoding';
import { Metadata } from 'next';
import { z } from 'zod';

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

  const gameType = await prisma.gameType.findUnique({
    select: {
      rankMethod: true,
      playerCount: true,
      playerInfoGameTypes: {
        select: {
          rating: true,
          playTime: true,
          playerName: true,
          player: {
            select: {
              clanName: true,
              lastSeenAt: true,
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
        skip: (page - 1) * 100,
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
      playerCount={gameType.playerCount}
      rankMethod={gameType.rankMethod === RankMethod.TIME ? null : gameType.rankMethod}
      players={gameType.playerInfoGameTypes.map((playerInfoGameType, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: playerInfoGameType.playerName,
        clan: playerInfoGameType.player.clanName ?? undefined,
        rating: playerInfoGameType.rating ?? undefined,
        playTime: playerInfoGameType.playTime,
        lastSeenAt: playerInfoGameType.player.lastSeenAt,
        gameServers: playerInfoGameType.player.gameServerStateClients.map((client) => ({
          ip: client.gameServerState.gameServer?.ip ?? '',
          port: client.gameServerState.gameServer?.port ?? 0,
        })),
      }))}
    />
  );
}
