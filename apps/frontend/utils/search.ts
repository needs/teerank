import { z } from 'zod';
import prisma from './prisma';
import { searchClans, searchPlayers } from '@prisma/client/sql';

const resultServerSchema = z.object({
  ip: z.string(),
  port: z.number(),
});

export async function search(query: string) {
  query = query.replace(/_/g, '\\_').replace(/%/g, '\\%');

  console.time('search players');

  const players = await prisma.$queryRawTyped(searchPlayers(`%${query}%`)).then((players) => {
    return players.map((player) => ({
      ...player,
      servers: player.servers?.map((server) => resultServerSchema.parse(server)) ?? [],
    }));
  });

  console.timeEnd('search players');
  console.time('search clans');

  const clans = await prisma.$queryRawTyped(searchClans(`%${query}%`)).then((clans) => {
    return clans.map((clan) => ({
      ...clan,
      playerCount: Number(clan.playerCount) || 0,
    }));
  });

  console.timeEnd('search clans');
  console.time('search game servers');

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

  console.timeEnd('search game servers');

  return { players, clans, gameServers };
}
