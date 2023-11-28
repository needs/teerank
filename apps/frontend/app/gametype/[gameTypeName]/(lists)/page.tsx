import { PlayerList } from '../../../../components/PlayerList';
import prisma from '../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from '../schema';
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
  const { gameTypeName } = paramsSchema.parse(params);

  const gameType = await prisma.gameType.findUnique({
    select: {
      rankMethod: true,
      _count: {
        select: {
          playerInfos: true,
        },
      },
      playerInfos: {
        select: {
          rating: true,
          playTime: true,
          playerName: true,
          player: {
            select: {
              clanName: true,
            },
          },
        },
        orderBy: [
          {
            rating: 'desc',
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
      playerCount={gameType._count.playerInfos}
      hideRating={gameType.rankMethod === null}
      players={gameType.playerInfos.map((playerInfo, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: playerInfo.playerName,
        clan: playerInfo.player.clanName ?? undefined,
        rating: playerInfo.rating ?? undefined,
        playTime: playerInfo.playTime,
      }))}
    />
  );
}
