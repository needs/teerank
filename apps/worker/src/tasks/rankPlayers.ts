import { RankMethod } from "@prisma/client";
import { prisma } from "../prisma";
import { rankPlayersElo } from "../rankMethods/rankPlayersElo";
import { rankPlayersTime } from "../rankMethods/rankPlayersTime";

export async function rankPlayers(rangeStart: number, rangeEnd: number) {
  const snapshots = await prisma.gameServerSnapshot.findMany({
    where: {
      id: {
        gte: rangeStart,
        lte: rangeEnd,
      },
      map: {
        gameType: {
          rankMethod: {
            not: null,
          },
        },
      },
      rankedAt: null,
    },
    select: {
      id: true,
      map: {
        select: {
          gameType: {
            select: {
              rankMethod: true,
            }
          },
        }
      }
    }
  });

  for (const { id, map: { gameType: { rankMethod }} } of snapshots) {
    switch (rankMethod) {
      case RankMethod.ELO:
        await rankPlayersElo(id);
        break;
      case RankMethod.TIME:
        await rankPlayersTime(id);
        break;
    }

    await prisma.gameServerSnapshot.update({
      where: {
        id,
      },
      data: {
        rankedAt: new Date(),
      },
    });
  }
}
