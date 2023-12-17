import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import { searchParamPageSchema } from '../../../utils/page';
import prisma from '../../../utils/prisma';
import { GameTypeList } from '../../../components/GameTypeList';

export const metadata = {
  title: 'Player',
  description: 'A Teeworlds player',
};

export default async function Index({
  params,
  searchParams,
}: {
  params: z.infer<typeof paramsSchema>;
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { playerName } = paramsSchema.parse(params);
  const { page } = searchParamPageSchema.parse(searchParams);

  const player = await prisma.player.findUnique({
    select: {
      _count: {
        select: {
          playerInfoGameTypes: true,
        },
      },

      playerInfoGameTypes: {
        select: {
          gameTypeName: true,
          playTime: true,
        },
        orderBy: {
          playTime: 'desc',
        },
      },
    },
    where: {
      name: playerName,
    },
  });

  if (player === null) {
    return notFound();
  }

  return (
    <GameTypeList
      gameTypeCount={player._count.playerInfoGameTypes}
      gameTypes={player.playerInfoGameTypes.map((playerInfoGameType, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: playerInfoGameType.gameTypeName ?? '',
        playTime: playerInfoGameType.playTime,
      }))}
    />
  );
}
