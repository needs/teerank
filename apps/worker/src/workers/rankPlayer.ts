import { RankMethod } from "@prisma/client";
import { prisma } from "../prisma";
import { rankPlayersElo } from "../rankMethods/rankPlayersElo";
import { rankPlayersTime } from "../rankMethods/rankPlayersTime";
import { processRankPlayerJobs, RankPlayerJobData } from "@teerank/teerank";

export async function rankPlayer(data: RankPlayerJobData) {
  const snapshot = await prisma.gameServerSnapshot.findUniqueOrThrow({
    where: {
      id: data.snapshotId,
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
  });

  switch (snapshot.map.gameType.rankMethod) {
    case RankMethod.ELO:
      await rankPlayersElo(snapshot.id);
      break;
    case RankMethod.TIME:
      await rankPlayersTime(snapshot.id);
      break;
  }
}

export async function startRankPlayerWorker() {
  return await processRankPlayerJobs(rankPlayer);
}
