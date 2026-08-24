import { paramsSchema } from '../schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import { Metadata } from 'next';
import prisma from '../../../../utils/prisma';
import { searchParamPageSchema } from '../../../../utils/page';
import { TeammateList } from '../../../../components/TeammateList';
import { encodeString } from '../../../../utils/encoding';
import { countTeammates, getTeammates } from '../../../../utils/teammates';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { playerName } = paramsSchema.parse(params);

  return {
    title: `Player ${playerName} - Teammates`,
    description: 'Players a Teeworlds player spent the most time with',
    alternates: {
      canonical: `https://teerank.io/player/${encodeString(playerName)}/teammates`,
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
  const { playerName } = paramsSchema.parse(params);
  const { page } = searchParamPageSchema.parse(searchParams);

  const player = await prisma.player.findUnique({
    select: {
      id: true,
    },
    where: {
      name: playerName,
    },
  });

  if (player === null) {
    return notFound();
  }

  const [teammates, teammateCount] = await Promise.all([
    getTeammates(player.id, { skip: (page - 1) * 100, take: 100 }),
    countTeammates(player.id),
  ]);

  return (
    <TeammateList
      teammates={teammates}
      pageCount={Math.ceil(teammateCount / 100)}
    />
  );
}
