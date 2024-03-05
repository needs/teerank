import { Map, PrismaClient } from '@prisma/client'
import { subMinutes } from "date-fns";

export async function nextMapToCount(prisma: PrismaClient) {
  return await prisma.map.findFirst({
    where: {
      countedAt: {
        lte: new Date(subMinutes(Date.now(), 1)),
      },
    },
    orderBy: {
      countedAt: 'asc',
    },
  });
}

export async function updateMapCounts(prisma: PrismaClient, map: Map) {
  if (map.countedAt > subMinutes(new Date(), 1)) {
    return map;
  }

  const mapWithCounts = await prisma.map.findUnique({
    where: {
      id: map.id
    },
    select: {
      _count: {
        select: {
          playerInfoMaps: true,
          clanInfoMaps: true,
        },
      },
    },
  });

  const gameServerCount = await prisma.gameServer.count({
    where: {
      AND: [
        { lastSnapshot: { isNot: null } },
        {
          lastSnapshot: {
            mapId: map.id
          },
        },
      ],
    },
  });

  if (mapWithCounts === null) {
    return map;
  }

  return await prisma.map.update({
    where: {
      id: map.id
    },
    data: {
      playerCount: mapWithCounts._count.playerInfoMaps,
      clanCount: mapWithCounts._count.clanInfoMaps,
      gameServerCount,
      countedAt: new Date(),
    },
  });
}
