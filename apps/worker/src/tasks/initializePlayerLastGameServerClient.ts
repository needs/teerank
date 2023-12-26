import { prisma } from "../prisma";

export async function initializePlayerLastGameServerClient() {
  const player = await prisma.player.findFirst({
    where: {
      lastGameServerClient: null,
      gameServerClients: {
        some: {},
      }
    },
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
  });

  if (player === null) {
    return false;
  }

  const gameServerClient = player.gameServerClients.at(0);

  if (gameServerClient === undefined) {
    console.warn(`Player ${player.name} has no game server client`);
    return true;
  }

  await prisma.player.update({
    where: {
      name: player.name,
      lastGameServerClient: null,
    },

    data: {
      lastGameServerClient: {
        connect: {
          id: gameServerClient.id,
        },
      },
    },
  }).catch(() => null);

  return true;
}
