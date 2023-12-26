import { RankMethod } from "@prisma/client";
import { prisma } from "../prisma";
import { rankPlayersElo } from "../rankMethods/rankPlayersElo";
import { rankPlayersTime } from "../rankMethods/rankPlayersTime";

export async function rankPlayers() {
  const snapshotCandidate = await prisma.gameServerSnapshot.findFirst({
    where: {
      rankingStartedAt: null,
    },
    select: {
      id: true,
    },
  });

  if (snapshotCandidate === null) {
    return false;
  }

  const snapshot = await prisma.gameServerSnapshot.update({
    where: {
      id: snapshotCandidate.id,
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

  if (snapshot === null) {
    return false;
  }

  switch (snapshot.map.gameType.rankMethod) {
    case RankMethod.ELO:
      await rankPlayersElo(snapshot.id);
      break;
    case RankMethod.TIME:
      await rankPlayersTime(snapshot.id);
      break;
  }

  await prisma.gameServerSnapshot.update({
    where: {
      id: snapshot.id,
    },
    data: {
      rankedAt: new Date(),
    },
  });

  return true;
}
