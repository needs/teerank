import { Map, PrismaClient } from '@prisma/client'
import { subMinutes } from "date-fns";

const MAP_COUNT_CACHE_DURATION = 1;

export function mapCountOldestDate() {
  return subMinutes(new Date(), MAP_COUNT_CACHE_DURATION)
}

export async function nextMapToCount(prisma: PrismaClient) {
  return await prisma.map.findFirst({
    where: {
      countedAt: {
        lte: mapCountOldestDate(),
      },
    },
    orderBy: {
      countedAt: 'asc',
    },
  });
}

export async function updateMapCountsIfOutdated(prisma: PrismaClient, map: Map) {
  if (map.countedAt > mapCountOldestDate()) {
    return map;
  }

  const [mapWithCounts, gameServerCount] = await Promise.all([
    prisma.map.update({
      select: {
        _count: {
          select: {
            playerInfoMaps: true,
            clanInfoMaps: true,
          },
        },
      },
      where: {
        id: map.id,
        countedAt: map.countedAt,
      },
      data: {
        countedAt: new Date(),
      },
    }).catch(() => null),

    prisma.gameServer.count({
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
    })
  ]);

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
    },
  });
}
