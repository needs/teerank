import prisma from '@teerank/frontend/utils/prisma';
import { paramsSchema, searchParamsSchema } from './schema';
import { PlayerList } from '@teerank/frontend/components/PlayerList';
import { notFound } from 'next/navigation';

export const metadata = {
  title: 'Players',
  description: 'List of ranked players',
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

  const playerInfos = await prisma.playerInfo.findMany({
    where: {
      map: {
        name: { equals: mapName },
        gameTypeName: { equals: gameTypeName }
      },
    },
    orderBy: [
      {
        rating: 'desc',
      },
      {
        playTime: 'desc',
      },
    ],
    include: {
      player: true,
    },
    take: 100,
    skip: (page - 1) * 100,
  });

  const map = await prisma.map.findUnique({
    where: {
      name_gameTypeName: {
        name: mapName,
        gameTypeName: gameTypeName,
      },
    },
    include: {
      gameType: true,
    }
  });

  if (map === null) {
    return notFound();
  }

  return (
    <PlayerList
      hideRating={map.gameType.rankMethod === null}
      players={playerInfos.map((playerInfo, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: playerInfo.player.name,
        clan: undefined,
        rating: playerInfo.rating ?? undefined,
        playTime: playerInfo.playTime,
      }))}
    />
  );
}
