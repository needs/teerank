import { NextRequest, NextResponse } from 'next/server';
import { z } from 'zod';
import { decodeIp } from '../../../../../../utils/encoding';
import prisma from '../../../../../../utils/prisma';
import { getServerActivity, rangeParamsFromSearch } from '../../../../../../utils/activity';

const paramsSchema = z.object({
  ip: z.string().transform(decodeIp),
  port: z.coerce.number().int().positive().max(65535),
});

export async function GET(
  request: NextRequest,
  { params }: { params: { ip: string; port: string } }
) {
  const parsedParams = paramsSchema.safeParse(params);

  if (!parsedParams.success) {
    return NextResponse.json({ error: 'Bad request' }, { status: 400 });
  }

  const { ip, port } = parsedParams.data;

  const gameServer = await prisma.gameServer.findUnique({
    where: { ip_port: { ip, port } },
    select: { id: true },
  });

  if (gameServer === null) {
    return NextResponse.json({ error: 'Not found' }, { status: 404 });
  }

  return NextResponse.json(
    await getServerActivity(gameServer.id, rangeParamsFromSearch(request.nextUrl.searchParams))
  );
}
