import { NextRequest, NextResponse } from 'next/server';
import { z } from 'zod';
import { decodeString } from '../../../../../../../utils/encoding';
import prisma from '../../../../../../../utils/prisma';
import { getMapActivity, rangeParamsFromSearch } from '../../../../../../../utils/activity';

const paramsSchema = z.object({
  gameTypeName: z.string().transform(decodeString),
  mapName: z.string().transform(decodeString),
});

export async function GET(
  request: NextRequest,
  { params }: { params: { gameTypeName: string; mapName: string } }
) {
  const parsedParams = paramsSchema.safeParse(params);

  if (!parsedParams.success) {
    return NextResponse.json({ error: 'Bad request' }, { status: 400 });
  }

  const map = await prisma.map.findUnique({
    where: {
      name_gameTypeName: {
        name: parsedParams.data.mapName,
        gameTypeName: parsedParams.data.gameTypeName,
      },
    },
    select: { id: true },
  });

  if (map === null) {
    return NextResponse.json({ error: 'Not found' }, { status: 404 });
  }

  return NextResponse.json(
    await getMapActivity(map.id, rangeParamsFromSearch(request.nextUrl.searchParams))
  );
}
