import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import prisma from '../../../utils/prisma';
import { PlayerList } from '../../../components/PlayerList';
import { searchParamPageSchema } from '../../../utils/page';
import { Metadata } from 'next';
import { encodeString } from '../../../utils/encoding';
import Link from 'next/link';
import { PastMembersToggle } from './PastMembersToggle';
export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { clanName } = paramsSchema.parse(params);

  return {
    title: `Clan ${clanName}`,
    description: `List of ranked players for ${clanName}`,
    alternates: {
      canonical: `https://teerank.io/clan/${encodeString(clanName)}`,
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
  const showPastMembers = searchParams.past === 'true' || searchParams.past === '1';

  const clan = await prisma.clan.findUnique({
    select: {
      activePlayerCount: true,
      _count: {
        select: {
          clanPlayerInfos: true,
        },
      },
      clanPlayerInfos: {
        select: {
          playTime: true,
          player: {
            select: {
              name: true,
              clanName: true,
              lastSeenAt: true,
              gameServerStateClients: {
                select: {
                  gameServerState: {
                    select: {
                      createdAt: true,
                      gameServer: {
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
        where: showPastMembers ? undefined : {
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
    <div className="flex flex-col gap-4">
      <PastMembersToggle clanName={clanName} showPastMembers={showPastMembers} />
      <PlayerList
        playerCount={showPastMembers ? clan._count.clanPlayerInfos : clan.activePlayerCount}
        rankMethod={null}
        showLastSeen={true}
        players={clan.clanPlayerInfos.map((playerInfo, index) => ({
          rank: index + 1,
          name: playerInfo.player.name,
          clan: clanName,
          isActiveClan: playerInfo.player.clanName === clanName,
          playTime: playerInfo.playTime,
          lastSeenAt: playerInfo.player.lastSeenAt,
          gameServers: playerInfo.player.gameServerStateClients.map((client) => ({
            ip: client.gameServerState.gameServer?.ip ?? '',
            port: client.gameServerState.gameServer?.port ?? 0,
          })),
        }))}
      />
    </div>
  );
}
