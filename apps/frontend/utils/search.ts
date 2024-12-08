import prisma from './prisma';

export async function search(query: string) {
  query = query.replace(/_/g, '\\_').replace(/%/g, '\\%');

  console.time('search players');

  const players = await prisma.player.findMany({
    where: {
      name: {
        contains: query,
        mode: 'insensitive',
      },
    },

    include: {
      gameServerStateClients: {
        include: {
          gameServerState: {
            include: {
              gameServer: true,
            },
          },
        },
      }
    },

    orderBy: {
      playTime: 'desc',
    },
  });

  console.timeEnd('search players');

  players.sort((a, b) => {
    const diffName = a.name.length - b.name.length;
    return diffName || Number(b.playTime) - Number(a.playTime);
  });

  //const query2 = `%${query}%`
  //const player2 = await prisma.$queryRaw`SELECT "public"."Player"."name", "public"."Player"."createdAt", "public"."Player"."updatedAt", "public"."Player"."lastSeenAt", "public"."Player"."clanName", "public"."Player"."clanSnapshotCreatedAt", "public"."Player"."playTime" FROM "public"."Player" WHERE "public"."Player"."name" ILIKE ${query2} ORDER BY LENGTH("public"."Player"."name"), "public"."Player"."playTime" DESC OFFSET 0`;
  //console.log(player2.map(p => p.name));

  const clans = await prisma.clan.findMany({
    where: {
      name: {
        contains: query,
        mode: 'insensitive',
      },
    },
    select: {
      name: true,
      _count: {
        select: {
          players: true,
        },
      },
      playTime: true,
    },
    take: 30,
  });

  const gameServers = await prisma.gameServerState.findMany({
    where: {
      name: {
        contains: query,
        mode: 'insensitive',
      },
    },
    take: 30,
    include: {
      gameServer: true,
      map: true,
    },
  });

  return { players, clans, gameServers };
}
