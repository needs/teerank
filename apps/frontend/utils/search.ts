import { z } from 'zod';
import { STUB_MIN_POLL_COUNT, STUB_OCCURRENCE_RATIO } from '@teerank/teerank';
import prisma from './prisma';
import { searchClans, searchGameServers, searchPlayers } from '@prisma/client/sql';

const resultServerSchema = z.object({
  ip: z.string(),
  port: z.number(),
});

export async function search(query: string, { includeShared = true } = {}) {
  query = query.replace(/_/g, '\\_').replace(/%/g, '\\%');

  console.time('search players');

  const players = await prisma.$queryRawTyped(
    searchPlayers(`%${query}%`, includeShared, STUB_MIN_POLL_COUNT, STUB_OCCURRENCE_RATIO)
  ).then((players) => {
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
      playerCount: Number(clan.activePlayerCount) || 0,
    }));
  });

  console.timeEnd('search clans');
  console.time('search game servers');

  const gameServers = await prisma.$queryRawTyped(searchGameServers(`%${query}%`)).then((gameServers) => {
    return gameServers.map((gameServer) => ({
      ...gameServer,
    }));
  });

  console.timeEnd('search game servers');

  return { players, clans, gameServers };
}
