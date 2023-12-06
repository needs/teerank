import { paramsSchema } from '../schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import prisma from '../../../../utils/prisma';
import { searchParamPageSchema } from '../../../../utils/page';
import { MapList } from '../../../../components/MapList';

export const metadata = {
  title: 'Clan',
  description: 'A Teeworlds clan',
};

export default async function Index({
  params,
  searchParams,
}: {
  params: z.infer<typeof paramsSchema>;
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { clanName } = paramsSchema.parse(params);
  const { page } = searchParamPageSchema.parse(searchParams);

  const clan = await prisma.clan.findUnique({
    select: {
      _count: {
        select: {
          clanInfos: {
            where: {
              map: {
                isNot: null,
              },
            },
          },
        },
      },

      clanInfos: {
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
      name: clanName,
    },
  });

  if (clan === null) {
    return notFound();
  }

  return (
    <MapList
      mapCount={clan._count.clanInfos}
      maps={clan.clanInfos.map((clanInfo, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: clanInfo.map?.name ?? '',
        gameTypeName: clanInfo.map?.gameTypeName ?? '',
        playTime: clanInfo.playTime,
      }))}
    />
  );
}
