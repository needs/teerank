import { Metadata } from 'next';
import { ClanList } from '../../../../../../components/ClanList';
import { encodeString } from '../../../../../../utils/encoding';
import prisma from '../../../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from '../schema';
import { notFound } from 'next/navigation';
import { z } from 'zod';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { gameTypeName, mapName } = paramsSchema.parse(params);

  return {
    title: `${mapName} - ${gameTypeName}`,
    description: `List of ranked clans for ${mapName} in ${gameTypeName}`,
    alternates: {
      canonical: `https://teerank.io/gametype/${encodeString(gameTypeName)}/map/${encodeString(mapName)}/clans`,
    },
  };
}

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
      clanCount: true,
      clanInfoMaps: {
        select: {
          clan: {
            select: {
              activePlayerCount: true,
            },
          },
          clanName: true,
          playTime: true,
        },
        orderBy: [
          {
            playTime: 'desc',
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
      clanCount={map.clanCount}
      clans={map.clanInfoMaps.map((clanInfoMap, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: clanInfoMap.clanName,
        playerCount: clanInfoMap.clan.activePlayerCount,
        playTime: clanInfoMap.playTime,
      }))}
    />
  );
}
