import { subMinutes } from "date-fns";
import prisma from "./prisma";

const globalId = "teerank";

export async function getGlobalCounts() {
  const globalCounts = await prisma.globalCounts.findUnique({
    where: {
      globalId,
    },
  });

  if (globalCounts !== null && globalCounts.updatedAt > subMinutes(new Date(), 1)) {
    return {
      playerCount: globalCounts.playerCount,
      clanCount: globalCounts.clanCount,
      gameServerCount: globalCounts.gameServerCount,
    };
  }

  const [playerCount, clanCount, gameServerCount] = await Promise.all([
    prisma.player.count(),
    prisma.clan.count(),
    prisma.gameServer.count({
      where: {
        lastSnapshot: {
          isNot: null,
        },
      },
    }),
  ]);

  await prisma.globalCounts.upsert({
    where: {
      globalId
    },
    update: {
      playerCount,
      clanCount,
      gameServerCount,
    },
    create: {
      globalId,
      playerCount,
      clanCount,
      gameServerCount,
    },
  }).catch(console.error);

  return {
    playerCount,
    clanCount,
    gameServerCount,
  };
}
