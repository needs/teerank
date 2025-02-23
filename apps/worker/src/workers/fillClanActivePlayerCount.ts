import { prisma } from "../prisma";
import { processFillClanActivePlayerCountJobs } from "@teerank/teerank";

export async function fillClanActivePlayerCount() {
  const clans = await prisma.clan.findMany({
    select: {
      name: true,
      activePlayerCount: true,
    },
  });

  const clansMap = new Map<string, number>();

  for (const clan of clans) {
    clansMap.set(clan.name, clan.activePlayerCount);
  }

  const playersByClan = await prisma.player.groupBy({
    by: ['clanName'],
    _count: {
      id: true,
    },
  });

  for (const player of playersByClan) {
    if (player.clanName !== null) {
      const oldCount = clansMap.get(player.clanName) || 0;
      const newCount = player._count.id;
      clansMap.delete(player.clanName);

      if (oldCount !== newCount) {
        await prisma.clan.update({
          where: { name: player.clanName },
          data: { activePlayerCount: newCount },
        });
      }
    }
  }

  // All clans that are still in the map means they have no active players
  // anymore.
  for (const [clanName, count] of clansMap) {
    if (count !== 0) {
      await prisma.clan.update({
        where: { name: clanName },
        data: { activePlayerCount: 0 },
      });
    }
  }
}

export async function startFillClanActivePlayerCountWorker() {
  return await processFillClanActivePlayerCountJobs(fillClanActivePlayerCount);
}
