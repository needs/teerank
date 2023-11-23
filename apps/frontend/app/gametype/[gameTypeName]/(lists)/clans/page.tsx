import prisma from '@teerank/frontend/utils/prisma';
import { paramsSchema, searchParamsSchema } from '../../schema';
import { PlayerList } from '@teerank/frontend/components/PlayerList';
import { notFound } from 'next/navigation';
import { ClanList } from '@teerank/frontend/components/ClanList';

export const metadata = {
  title: 'Clans',
  description: 'List of ranked clans',
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

  const clansInfos = await prisma.clanInfo.findMany({
    select: {
      clan: {
        include: {
          _count: {
            select: {
              players: true,
            },
          },
        },
      },
      playTime: true,
    },
    where: {
      gameType: {
        name: { equals: gameTypeName },
      },
    },
    orderBy: [
      {
        playTime: 'desc',
      },
      {
        clan: {
          players: {
            _count: 'desc',
          },
        },
      },
    ],
    take: 100,
    skip: (page - 1) * 100,
  });

  const gameType = await prisma.gameType.findUnique({
    where: {
      name: gameTypeName,
    },
  });

  if (gameType === null) {
    return notFound();
  }

  return (
    <ClanList
      clans={clansInfos.map((clanInfo, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: clanInfo.clan.name,
        playerCount: clanInfo.clan._count.players,
        playTime: clanInfo.playTime,
      }))}
    />
  );
}
