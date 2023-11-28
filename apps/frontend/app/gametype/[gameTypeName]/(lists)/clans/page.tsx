import { ClanList } from '../../../../../components/ClanList';
import prisma from '../../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from '../../schema';
import { notFound } from 'next/navigation';

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

  const gameType = await prisma.gameType.findUnique({
    where: {
      name: gameTypeName,
    },
    select: {
      _count: {
        select: {
          clanInfos: true,
        },
      },
      clanInfos: {
        select: {
          clan: {
            select: {
              _count: {
                select: {
                  players: true,
                },
              },
            },
          },
          clanName: true,
          playTime: true,
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
      },
    },
  });

  if (gameType === null) {
    return notFound();
  }

  return (
    <ClanList
      clanCount={gameType._count.clanInfos}
      clans={gameType.clanInfos.map((clanInfo, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: clanInfo.clanName,
        playerCount: clanInfo.clan._count.players,
        playTime: clanInfo.playTime,
      }))}
    />
  );
}
