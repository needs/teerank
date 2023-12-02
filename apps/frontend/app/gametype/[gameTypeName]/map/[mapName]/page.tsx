import { PlayerList } from '../../../../../components/PlayerList';
import prisma from '../../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from './schema';
import { notFound } from 'next/navigation';

export const metadata = {
  title: 'Players',
  description: 'List of ranked players',
};

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
      _count: {
        select: {
          playerInfos: true,
        },
      },
      playerInfos: {
        select: {
          rating: true,
          player: {
            select: {
              name: true,
              clanName: true,
            }
          },
          playTime: true,
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
      }
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
    playerCount={map._count.playerInfos}
      rankMethod={map.gameType.rankMethod}
      players={map.playerInfos.map((playerInfo, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: playerInfo.player.name,
        clan: playerInfo.player.clanName ?? undefined,
        rating: playerInfo.rating ?? undefined,
        playTime: playerInfo.playTime,
      }))}
    />
  );
}
