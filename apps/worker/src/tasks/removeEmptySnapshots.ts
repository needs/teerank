import { prisma } from "../prisma";

export async function removeEmptySnapshots() {
  // Because of a bug in the game server polling logic, some empty snapshots are
  // created empty. They needs to be removed and server marked as offline
  // properly.

  const emptySnapshot = await prisma.gameServerSnapshot.findFirst({
    where: {
      version: ""
    },
    select: {
      id: true,
      createdAt: true,
      gameServerLast: {
        select: {
          id: true,
          offlineSince: true,
        },
      },
    },
  });

  if (emptySnapshot === null) {
    return false;
  }

  // If snapshot was the last one, get the latest non empty snapshot and assign
  // it as last, update offlineSince as well if it was null or set at a later
  // date.
  if (emptySnapshot.gameServerLast !== null) {
    const lastNonEmptySnapshot = await prisma.gameServerSnapshot.findFirst({
      where: {
        gameServerId: emptySnapshot.gameServerLast.id,
        version: {
          not: "",
        },
      },
      orderBy: {
        createdAt: 'desc',
      },
      select: {
        id: true,
        createdAt: true,
      },
    });

    if (lastNonEmptySnapshot !== null) {
      let offlineSince = emptySnapshot.gameServerLast.offlineSince;

      if (offlineSince === null || lastNonEmptySnapshot.createdAt < offlineSince) {
        offlineSince = lastNonEmptySnapshot.createdAt;
      }

      await prisma.gameServer.update({
        where: {
          id: emptySnapshot.gameServerLast.id,
        },
        data: {
          lastSnapshot: {
            connect: {
              id: lastNonEmptySnapshot.id,
            },
          },
          offlineSince,
        },
      });
    } else {
      let offlineSince = emptySnapshot.gameServerLast.offlineSince;

      if (offlineSince === null || emptySnapshot.createdAt < offlineSince) {
        offlineSince = emptySnapshot.createdAt;
      }

      await prisma.gameServer.update({
        where: {
          id: emptySnapshot.gameServerLast.id,
        },
        data: {
          offlineSince,
          lastSnapshot: {
            disconnect: true,
          }
        },
      });
    }
  }

  await prisma.gameServerSnapshot.delete({
    where: {
      id: emptySnapshot.id,
    },
  });

  return true;
}
