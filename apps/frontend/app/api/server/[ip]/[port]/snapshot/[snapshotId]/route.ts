import { NextResponse } from 'next/server';
import { z } from 'zod';
import { decodeIp } from '../../../../../../../utils/encoding';
import prisma from '../../../../../../../utils/prisma';

const paramsSchema = z.object({
  ip: z.string().transform(decodeIp),
  port: z.coerce.number().int().positive().max(65535),
  snapshotId: z.coerce.number().int().positive(),
});

export async function GET(
  _request: Request,
  { params }: { params: { ip: string; port: string; snapshotId: string } }
) {
  const parsedParams = paramsSchema.safeParse(params);

  if (!parsedParams.success) {
    return NextResponse.json({ error: 'Bad request' }, { status: 400 });
  }

  const { ip, port, snapshotId } = parsedParams.data;

  const snapshot = await prisma.gameServerSnapshot.findFirst({
    where: {
      id: snapshotId,
      gameServer: {
        ip,
        port,
      },
    },
    select: {
      id: true,
      createdAt: true,
      name: true,
      numClients: true,
      maxClients: true,
      map: {
        select: {
          name: true,
          gameTypeName: true,
        },
      },
      clients: {
        orderBy: {
          score: 'desc',
        },
        select: {
          playerName: true,
          clanName: true,
          score: true,
        },
      },
    },
  });

  if (snapshot === null) {
    return NextResponse.json({ error: 'Not found' }, { status: 404 });
  }

  return NextResponse.json(snapshot, {
    headers: {
      'Cache-Control': 'public, max-age=86400, immutable',
    },
  });
}
