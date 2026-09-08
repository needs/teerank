import { Prisma, PrismaClient } from "@prisma/client";
import { prisma as defaultPrisma } from "./prisma";

const snapshotSelect = {
  id: true,
  createdAt: true,
  gameServerId: true,
  mapId: true,
  numPlayers: true,
  maxPlayers: true,
  numClients: true,
  maxClients: true,
  map: {
    select: {
      name: true,
      gameTypeName: true,
    },
  },
  clients: {
    select: {
      playerName: true,
      clanName: true,
      score: true,
      country: true,
      inGame: true,
    },
  },
} satisfies Prisma.GameServerSnapshotSelect;

export type IteratedSnapshot = Prisma.GameServerSnapshotGetPayload<{
  select: typeof snapshotSelect;
}>;

export async function* iterateSnapshots({
  from,
  to,
  batchSize,
  prisma = defaultPrisma,
}: {
  from: Date;
  to: Date;
  batchSize: number;
  prisma?: Pick<PrismaClient, 'gameServerSnapshot'>;
}): AsyncGenerator<IteratedSnapshot, void, undefined> {
  let cursor = 0;

  for (;;) {
    const snapshots = await prisma.gameServerSnapshot.findMany({
      where: {
        createdAt: { gte: from, lt: to },
        id: { gt: cursor },
      },
      orderBy: {
        id: 'asc',
      },
      take: batchSize,
      select: snapshotSelect,
    });

    if (snapshots.length === 0) {
      return;
    }

    yield* snapshots;
    cursor = snapshots[snapshots.length - 1].id;

    if (snapshots.length < batchSize) {
      return;
    }
  }
}
