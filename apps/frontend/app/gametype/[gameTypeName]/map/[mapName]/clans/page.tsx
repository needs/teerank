import { ClanList } from '../../../../../../components/ClanList';
import prisma from '../../../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from '../schema';
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
  const { gameTypeName, mapName } = paramsSchema.parse(params);

  const map = await prisma.map.findUnique({
    select: {
      _count: {
        select: {
          clanInfoMaps: true,
        }
      },
      clanInfoMaps: {
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
    where: {
      name_gameTypeName: {
        name: mapName,
        gameTypeName: gameTypeName,
      },
    },
  });

  if (map === null) {
    return notFound();
  }

  return (
    <ClanList
      clanCount={map._count.clanInfoMaps}
      clans={map.clanInfoMaps.map((clanInfoMap, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: clanInfoMap.clanName,
        playerCount: clanInfoMap.clan._count.players,
        playTime: clanInfoMap.playTime,
      }))}
    />
  );
}
