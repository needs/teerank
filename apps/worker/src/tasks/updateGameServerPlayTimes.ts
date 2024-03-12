import { prisma } from "../prisma";
import { differenceInSeconds } from "date-fns";
import { removeDuplicatedClients } from "../utils";

export async function updateGameServerPlayTimes() {
  const snapshotCandidate = await prisma.gameServerSnapshot.findFirst({
    where: {
      gameServerPlayTimingStartedAt: null,
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
      gameServerPlayTimingStartedAt: null,
    },
    select: {
      id: true,
      createdAt: true,
      gameServerId: true,
      gameServerPlayTimedAt: true,
      clients: {
        select: {
          playerName: true,
        },
      },
    },
    data: {
      gameServerPlayTimingStartedAt: new Date(),
    },
  }).catch(() => null);

  if (snapshot === null) {
    return true;
  }

  if (snapshot.gameServerPlayTimedAt === null) {
    const snapshotBefore = await prisma.gameServerSnapshot.findFirst({
      select: {
        createdAt: true,
      },
      where: {
        createdAt: {
          lt: snapshot.createdAt,
        },
        gameServerId: snapshot.gameServerId,
      },
      orderBy: {
        createdAt: 'desc',
      },
    });

    const deltaSecond = snapshotBefore === null ? 0 : differenceInSeconds(snapshot.createdAt, snapshotBefore.createdAt);
    const deltaPlayTime = deltaSecond > 10 * 60 ? 5 * 60 : deltaSecond;
    const clients = removeDuplicatedClients(snapshot.clients);

    await prisma.gameServer.update({
      where: {
        id: snapshot.gameServerId,
      },
      data: {
        playTime: {
          increment: deltaPlayTime * clients.length,
        },
      },
    });
  }

  await prisma.gameServerSnapshot.update({
    where: {
      id: snapshot.id,
    },
    data: {
      gameServerPlayTimedAt: new Date(),
    },
  });

  return true;
}
