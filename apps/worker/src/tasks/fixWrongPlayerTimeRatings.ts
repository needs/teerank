import { RankMethod } from "@prisma/client";
import { prisma } from "../prisma";

export async function fixWrongPlayerTimeRatings() {
  const playerInfoMap = await prisma.playerInfoMap.findFirst({
    where: {
      rating: { gte: 0 },
      map: {
        gameType: {
          rankMethod: RankMethod.TIME,
        }
      }
    },
  });

  if (playerInfoMap === null) {
    return false;
  }

  const lock = await prisma.playerInfoMap.update({
    where: {
      id: playerInfoMap.id,
    },
    data: {
      rating: null,
    }
  }).catch(() => null);

  if (lock === null) {
    return true;
  }

  const gameServerClient = await prisma.gameServerClient.findFirst({
    where: {
      inGame: true,
      score: { not: -9999 },
      playerName: playerInfoMap.playerName,
      snapshot: {
        map: {
          id: playerInfoMap.mapId,
        }
      }
    },
    select: {
      id: true,
      score: true,
    },
    orderBy: {
      score: 'desc',
    },
  });

  if (gameServerClient === null) {
    return true;
  }

  const rating = -Math.abs(gameServerClient.score);

  if (rating < 0) {
    await prisma.playerInfoMap.update({
      where: {
        id: playerInfoMap.id,
      },
      data: {
        rating
      }
    });
  }

  return true;
}
