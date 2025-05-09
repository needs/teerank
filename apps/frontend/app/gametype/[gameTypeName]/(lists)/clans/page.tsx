import { Metadata } from 'next';
import { ClanList } from '../../../../../components/ClanList';
import { encodeString } from '../../../../../utils/encoding';
import prisma from '../../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from '../../schema';
import { notFound } from 'next/navigation';
import { z } from 'zod';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { gameTypeName } = paramsSchema.parse(params);

  return {
    title: `Gametype ${gameTypeName}`,
    description: `List of ranked clans for ${gameTypeName}`,
    alternates: {
      canonical: `https://teerank.io/gametype/${encodeString(gameTypeName)}/clans`,
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
  const { gameTypeName } = paramsSchema.parse(params);

  const gameType = await prisma.gameType.findUnique({
    where: {
      name: gameTypeName,
    },
    select: {
      clanCount: true,
      clanInfoGameTypes: {
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
  });

  if (gameType === null) {
    return notFound();
  }

  return (
    <ClanList
      clanCount={gameType.clanCount}
      clans={gameType.clanInfoGameTypes.map((clanInfoGameType, index) => ({
        rank: (page - 1) * 100 + index + 1,
        name: clanInfoGameType.clanName,
        playerCount: clanInfoGameType.clan.activePlayerCount,
        playTime: clanInfoGameType.playTime,
      }))}
    />
  );
}
