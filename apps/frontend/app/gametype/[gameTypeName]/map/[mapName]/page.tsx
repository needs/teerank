import { Metadata } from 'next';
import { PlayerList } from '../../../../../components/PlayerList';
import { encodeString } from '../../../../../utils/encoding';
import prisma from '../../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from './schema';
import { notFound } from 'next/navigation';
import { z } from 'zod';

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

  const map = await prisma.map.findUnique({
    select: {
      gameType: {
        select: {
          rankMethod: true,
        },
      },
      playerCount: true,
      playerInfoMaps: {
        select: {
          rating: true,
          player: {
            select: {
              name: true,
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
        skip: (page - 1) * 100,
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
      playerCount={map.playerCount}
      rankMethod={map.gameType.rankMethod}
      players={map.playerInfoMaps.map((playerInfoMap, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: playerInfoMap.player.name,
        clan: playerInfoMap.player.clanName ?? undefined,
        rating: playerInfoMap.rating ?? undefined,
        playTime: playerInfoMap.playTime,
        lastSeenAt: playerInfoMap.player.lastSeenAt,
        gameServers: playerInfoMap.player.gameServerStateClients.map((client) => ({
          ip: client.gameServerState.gameServer?.ip ?? '',
          port: client.gameServerState.gameServer?.port ?? 0,
        })),
      }))}
    />
  );
}
