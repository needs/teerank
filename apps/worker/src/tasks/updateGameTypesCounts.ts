import { subMinutes } from "date-fns";
import { prisma } from "../prisma";

export async function updateGameTypesCounts() {
  const gameType = await prisma.gameType.findFirst({
    where: {
      countedAt: {
        lte: new Date(subMinutes(Date.now(), 1)),
      },
    },
  });

  if (gameType === null) {
    return false;
  }

  const gameTypeWithCounts = await prisma.gameType.findUnique({
    where: {
      name: gameType.name,
    },
    select: {
      _count: {
        select: {
          playerInfoGameTypes: true,
          map: true,
          clanInfoGameTypes: true,
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
            map: {
              gameTypeName: gameType.name,
            },
          },
        },
      ],
    },
  });

  if (gameTypeWithCounts === null) {
    return false;
  }

  await prisma.gameType.update({
    where: {
      name: gameType.name,
    },
    data: {
      playerCount: gameTypeWithCounts._count.playerInfoGameTypes,
      mapCount: gameTypeWithCounts._count.map,
      clanCount: gameTypeWithCounts._count.clanInfoGameTypes,
      gameServerCount,
      countedAt: new Date(),
    },
  });

  return true;
}
