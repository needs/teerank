import { GameType, PrismaClient } from '@prisma/client'
import { subMinutes } from "date-fns";

export async function nextGameTypeToCount(prisma: PrismaClient) {
  return await prisma.gameType.findFirst({
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

export async function updateGameTypeCountsIfOutdated(prisma: PrismaClient, gameType: GameType) {
  if (gameType.countedAt > subMinutes(new Date(), 1)) {
    return gameType;
  }

  const [gameTypeWithCounts, gameServerCount] = await Promise.all([
    prisma.gameType.update({
      select: {
        _count: {
          select: {
            playerInfoGameTypes: true,
            map: true,
            clanInfoGameTypes: true,
          },
        },
      },
      where: {
        name: gameType.name,
        countedAt: gameType.countedAt,
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
              map: {
                gameTypeName: gameType.name,
              },
            },
          },
        ],
      },
    })
  ]);

  if (gameTypeWithCounts === null) {
    return gameType;
  }

  return await prisma.gameType.update({
    where: {
      name: gameType.name
    },
    data: {
      playerCount: gameTypeWithCounts._count.playerInfoGameTypes,
      mapCount: gameTypeWithCounts._count.map,
      clanCount: gameTypeWithCounts._count.clanInfoGameTypes,
      gameServerCount,
    },
  });
}
