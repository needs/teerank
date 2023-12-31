import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import prisma from '../../../utils/prisma';
import { PlayerList } from '../../../components/PlayerList';

export const metadata = {
  title: 'Clan',
  description: 'A Teeworlds clan',
};

export default async function Index({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}) {
  const { clanName } = paramsSchema.parse(params);

  const clan = await prisma.clan.findUnique({
    select: {
      _count: {
        select: {
          players: true,
        },
      },
      clanPlayerInfos: {
        select: {
          playerName: true,
          playTime: true,
        },
        where: {
          player: {
            clanName
          }
        },
        orderBy: {
          playTime: 'desc',
        }
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
    <PlayerList
      playerCount={clan._count.players}
      rankMethod={null}
      players={clan.clanPlayerInfos.map((player, index) => ({
        rank: index + 1,
        name: player.playerName,
        clan: clanName,
        playTime: player.playTime,
      }))}
    />
  );
}
