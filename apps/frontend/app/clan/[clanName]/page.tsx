import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import prisma from '../../../utils/prisma';
import { PlayerList } from '../../../components/PlayerList';
import { searchParamPageSchema } from '../../../utils/page';

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
          players: true,
        },
      },
      clanPlayerInfos: {
        select: {
          playTime: true,
          player: {
            select: {
              name: true,
              lastGameServerClient: {
                select: {
                  snapshot: {
                    select: {
                      createdAt: true,
                      gameServerLast: {
                        select: {
                          ip: true,
                          port: true,
                        },
                      },
                    },
                  },
                },
              },
            },
          },
        },
        where: {
          player: {
            clanName,
          },
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
    <PlayerList
      playerCount={clan._count.players}
      rankMethod={null}
      showLastSeen={true}
      players={clan.clanPlayerInfos.map((playerInfo, index) => ({
        rank: index + 1,
        name: playerInfo.player.name,
        clan: clanName,
        playTime: playerInfo.playTime,
        lastSeen:
          playerInfo.player.lastGameServerClient === null
            ? undefined
            : {
                at: playerInfo.player.lastGameServerClient.snapshot.createdAt,
                lastSnapshot:
                  playerInfo.player.lastGameServerClient.snapshot
                    .gameServerLast,
              },
      }))}
    />
  );
}
