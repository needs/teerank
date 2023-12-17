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
          playerInfoMaps: true,
        },
      },

      playerInfoMaps: {
        select: {
          map: {
            select: {
              name: true,
              gameTypeName: true,
            },
          },
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
    <MapList
      mapCount={player._count.playerInfoMaps}
      maps={player.playerInfoMaps.map((playerInfoMap, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: playerInfoMap.map?.name ?? '',
        gameTypeName: playerInfoMap.map?.gameTypeName ?? '',
        playTime: playerInfoMap.playTime,
      }))}
    />
  );
}
