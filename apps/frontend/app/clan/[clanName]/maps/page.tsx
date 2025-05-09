import { paramsSchema } from '../schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import prisma from '../../../../utils/prisma';
import { searchParamPageSchema } from '../../../../utils/page';
import { MapList } from '../../../../components/MapList';
import { encodeString } from '../../../../utils/encoding';
import { Metadata } from 'next';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { clanName } = paramsSchema.parse(params);

  return {
    title: `Clan ${clanName} - Maps`,
    description: `List of ranked maps for ${clanName}`,
    alternates: {
      canonical: `https://teerank.io/clan/${encodeString(clanName)}/maps`,
    },
  };
}

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
          clanInfoMaps: true,
        },
      },

      clanInfoMaps: {
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
        take: 100,
        skip: (page - 1) * 100,
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
      mapCount={clan._count.clanInfoMaps}
      maps={clan.clanInfoMaps.map((clanInfoMap, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: clanInfoMap.map?.name ?? '',
        gameTypeName: clanInfoMap.map?.gameTypeName ?? '',
        playTime: clanInfoMap.playTime,
      }))}
    />
  );
}
