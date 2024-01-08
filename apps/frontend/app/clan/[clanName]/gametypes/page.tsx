import { paramsSchema } from '../schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import prisma from '../../../../utils/prisma';
import { GameTypeList } from '../../../../components/GameTypeList';
import { searchParamPageSchema } from '../../../../utils/page';

export const metadata = {
  title: 'Clan - Game types',
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
          clanInfoGameTypes: true,
        },
      },

      clanInfoGameTypes: {
        select: {
          gameTypeName: true,
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
    <GameTypeList
      gameTypeCount={clan._count.clanInfoGameTypes}
      gameTypes={clan.clanInfoGameTypes.map((clanInfoGameType, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: clanInfoGameType.gameTypeName ?? '',
        playTime: clanInfoGameType.playTime,
      }))}
    />
  );
}
