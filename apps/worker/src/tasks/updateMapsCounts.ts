import { subMinutes } from "date-fns";
import { prisma } from "../prisma";

export async function updateMapsCounts() {
  const map = await prisma.map.findFirst({
    where: {
      countedAt: {
        lte: new Date(subMinutes(Date.now(), 1)),
      },
    },
    orderBy: {
      countedAt: 'asc',
    },
  });

  if (map === null) {
    return false;
  }

  const mapWithCounts = await prisma.map.findUnique({
    where: {
      id: map.id,
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
            mapId: map.id,
          },
        },
      ],
    },
  });

  if (mapWithCounts === null) {
    return false;
  }

  await prisma.map.update({
    where: {
      id: map.id,
    },
    data: {
      playerCount: mapWithCounts._count.playerInfoMaps,
      clanCount: mapWithCounts._count.clanInfoMaps,
      gameServerCount,
      countedAt: new Date(),
    },
  });

  return true;
}
