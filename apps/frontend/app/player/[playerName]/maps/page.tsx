import { paramsSchema } from '../schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import { searchParamPageSchema } from '../../../../utils/page';
import prisma from '../../../../utils/prisma';
import { MapList } from '../../../../components/MapList';

export const metadata = {
  title: 'Player - Maps',
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
          playerInfos: {
            where: {
              map: {
                isNot: null,
              },
            },
          },
        },
      },

      playerInfos: {
        select: {
          map: {
            select: {
              name: true,
              gameTypeName: true,
            },
          },
          playTime: true,
        },
        where: {
          map: {
            isNot: null,
          },
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
    <MapList
      mapCount={player._count.playerInfos}
      maps={player.playerInfos.map((playerInfo, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: playerInfo.map?.name ?? '',
        gameTypeName: playerInfo.map?.gameTypeName ?? '',
        playTime: playerInfo.playTime,
      }))}
    />
  );
}
