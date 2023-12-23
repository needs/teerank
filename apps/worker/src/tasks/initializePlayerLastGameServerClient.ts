import { prisma } from "../prisma";

export async function initializePlayerLastGameServerClient(rangeStart: number, rangeEnd: number) {
  const players = await prisma.player.findMany({
    select: {
      name: true,

      lastGameServerClient: {
        select: {
          id: true,
          snapshot: {
            select: {
              createdAt: true,
            },
          }
        },
      },

      gameServerClients: {
        select: {
          id: true,
          snapshot: {
            select: {
              createdAt: true,
            },
          }
        },

        orderBy: {
          snapshot: {
            createdAt: 'desc',
          },
        },

        take: 1,
      },
    },

    where: {
      lastGameServerClientInitializedAt: null,
    },

    orderBy: {
      createdAt: 'desc',
    },

    take: rangeEnd - rangeStart + 1,
    skip: rangeStart,
  });

  for (const player of players) {
    const gameServerClient = player.gameServerClients[0];

    // Avoid race condition where player just got created and has no clients.
    if (gameServerClient === undefined) {
      continue;
    }

    // Avoid an hypothetical case where player has an already existing
    // lastGameServerClient that is somehow better than the one just found.
    if (player.lastGameServerClient !== null && player.lastGameServerClient.snapshot.createdAt > gameServerClient.snapshot.createdAt) {
      // Fix it by marking it as initialized.
      await prisma.player.update({
        where: {
          name: player.name,
        },

        data: {
          lastGameServerClientInitializedAt: new Date(),
        },
      });
      continue;
    }

    await prisma.player.update({
      where: {
        name: player.name,
      },

      data: {
        lastGameServerClient: {
          connect: {
            id: gameServerClient.id,
          },
        },
        lastGameServerClientInitializedAt: new Date(),
      },
    });
  }
}
