import { NextRequest, NextResponse } from 'next/server';
import { z } from 'zod';
import { decodeString } from '../../../../../utils/encoding';
import prisma from '../../../../../utils/prisma';
import { getPlayerActivity, rangeParamsFromSearch } from '../../../../../utils/activity';

const paramsSchema = z.object({
  playerName: z.string().transform(decodeString),
});

export async function GET(
  request: NextRequest,
  { params }: { params: { playerName: string } }
) {
  const parsedParams = paramsSchema.safeParse(params);

  if (!parsedParams.success) {
    return NextResponse.json({ error: 'Bad request' }, { status: 400 });
  }

  const player = await prisma.player.findUnique({
    where: { name: parsedParams.data.playerName },
    select: { id: true },
  });

  if (player === null) {
    return NextResponse.json({ error: 'Not found' }, { status: 404 });
  }

  return NextResponse.json(
    await getPlayerActivity(player.id, rangeParamsFromSearch(request.nextUrl.searchParams))
  );
}
