import { paramsSchema } from '../schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import prisma from '../../../../utils/prisma';
import { GameTypeList } from '../../../../components/GameTypeList';
import { searchParamPageSchema } from '../../../../utils/page';

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
              gameType: {
                isNot: null,
              },
            },
          },
        },
      },

      clanInfos: {
        select: {
          gameTypeName: true,
          playTime: true,
        },
        where: {
          gameType: {
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
    <GameTypeList
      gameTypeCount={clan._count.clanInfos}
      gameTypes={clan.clanInfos.map((clanInfo, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: clanInfo.gameTypeName ?? '',
        playTime: clanInfo.playTime,
      }))}
    />
  );
}
