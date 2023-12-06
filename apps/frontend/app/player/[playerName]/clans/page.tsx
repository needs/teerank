import { paramsSchema } from '../schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import { searchParamPageSchema } from '../../../../utils/page';
import prisma from '../../../../utils/prisma';
import { ClanList } from '../../../../components/ClanList';

export const metadata = {
  title: 'Player - Clans',
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
          clanPlayerInfos: {
            where: {
              playerName,
            },
          },
        },
      },

      clanPlayerInfos: {
        select: {
          clanName: true,
          playTime: true,
          clan: {
            select: {
              _count: {
                select: {
                  players: true,
                },
              },
            },
          },
        },
        where: {
          playerName,
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
    <ClanList
      clanCount={player._count.clanPlayerInfos}
      clans={player.clanPlayerInfos.map((clanPlayerInfo, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: clanPlayerInfo.clanName,
        playerCount: clanPlayerInfo.clan._count.players,
        playTime: clanPlayerInfo.playTime,
      }))}
    />
  );
}
