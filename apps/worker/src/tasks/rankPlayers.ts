import { RankMethod } from "@prisma/client";
import { prisma } from "../prisma";
import { rankPlayersElo } from "../rankMethods/rankPlayersElo";
import { rankPlayersTime } from "../rankMethods/rankPlayersTime";
import { compact } from "lodash";

export async function rankPlayers() {
  const snapshotCandidates = await prisma.gameServerSnapshot.findMany({
    where: {
      rankingStartedAt: null,
    },
    select: {
      id: true,
    },
    orderBy: {
      createdAt: 'desc',
    },
    take: 100,
  });

  if (snapshotCandidates.length === 0) {
    return false;
  }

  const snapshots = compact(await Promise.all(snapshotCandidates.map(async ({ id }) => {
    return await prisma.gameServerSnapshot.update({
      where: {
        id,
        rankingStartedAt: null,
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
      },
      data: {
        rankingStartedAt: new Date(),
      },
    }).catch(() => null);
  })));

  if (snapshots.length === 0) {
    return false;
  }

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

  return true;
}
