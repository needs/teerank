import { NextRequest, NextResponse } from 'next/server';
import { z } from 'zod';
import { decodeString } from '../../../../../utils/encoding';
import prisma from '../../../../../utils/prisma';
import { getGameTypeActivity, rangeParamsFromSearch } from '../../../../../utils/activity';

const paramsSchema = z.object({
  gameTypeName: z.string().transform(decodeString),
});

export async function GET(
  request: NextRequest,
  { params }: { params: { gameTypeName: string } }
) {
  const parsedParams = paramsSchema.safeParse(params);

  if (!parsedParams.success) {
    return NextResponse.json({ error: 'Bad request' }, { status: 400 });
  }

  const gameType = await prisma.gameType.findUnique({
    where: { name: parsedParams.data.gameTypeName },
    select: { id: true },
  });

  if (gameType === null) {
    return NextResponse.json({ error: 'Not found' }, { status: 404 });
  }

  return NextResponse.json(
    await getGameTypeActivity(gameType.id, rangeParamsFromSearch(request.nextUrl.searchParams))
  );
}
