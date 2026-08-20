import { NextRequest, NextResponse } from 'next/server';
import { z } from 'zod';
import { decodeString } from '../../../../../utils/encoding';
import prisma from '../../../../../utils/prisma';
import { getClanActivity, rangeParamsFromSearch } from '../../../../../utils/activity';

const paramsSchema = z.object({
  clanName: z.string().transform(decodeString),
});

export async function GET(
  request: NextRequest,
  { params }: { params: { clanName: string } }
) {
  const parsedParams = paramsSchema.safeParse(params);

  if (!parsedParams.success) {
    return NextResponse.json({ error: 'Bad request' }, { status: 400 });
  }

  const clan = await prisma.clan.findUnique({
    where: { name: parsedParams.data.clanName },
    select: { id: true },
  });

  if (clan === null) {
    return NextResponse.json({ error: 'Not found' }, { status: 404 });
  }

  return NextResponse.json(
    await getClanActivity(clan.id, rangeParamsFromSearch(request.nextUrl.searchParams))
  );
}
