import { RankMethod } from "@prisma/client";
import { prisma } from "../prisma";
import { rankPlayersElo } from "../rankMethods/rankPlayersElo";
import { rankPlayersTime } from "../rankMethods/rankPlayersTime";
import { Worker } from "bullmq";
import { bullmqConnection, QUEUE_NAME_RANK_PLAYER } from "@teerank/teerank";

export async function rankPlayer(snapshotId: number) {
  const snapshot = await prisma.gameServerSnapshot.findUniqueOrThrow({
    where: {
      id: snapshotId,
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
  new Worker(QUEUE_NAME_RANK_PLAYER, (job) => rankPlayer(job.data.snapshotId), {
    connection: bullmqConnection,
  });
}
