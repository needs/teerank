import { Redis } from "ioredis";

const GLOBAL_COUNTS_PLAYERS_LAST_ID_KEY = 'global-counts-players-last-id';
const GLOBAL_COUNTS_CLANS_LAST_ID_KEY = 'global-counts-clans-last-id';
const GLOBAL_COUNTS_GAME_TYPES_LAST_ID_KEY = 'global-counts-game-types-last-id';
const GLOBAL_COUNTS_MAPS_LAST_ID_KEY = 'global-counts-maps-last-id';
const GLOBAL_COUNTS_GAME_SERVERS_LAST_ID_KEY = 'global-counts-game-servers-last-id';

const GLOBAL_COUNTS_PLAYERS_KEY = 'global-counts-players-id';
const GLOBAL_COUNTS_CLANS_KEY = 'global-counts-clans-id';
const GLOBAL_COUNTS_GAME_TYPES_KEY = 'global-counts-game-types-id';
const GLOBAL_COUNTS_MAPS_KEY = 'global-counts-maps-id';
const GLOBAL_COUNTS_GAME_SERVERS_KEY = 'global-counts-game-servers-id';

export async function getGlobalCounts(redis: Redis) {
  const [
    playersStr,
    clansStr,
    mapsStr,
    gameTypesStr,
    gameServersStr
  ] = await redis.mget([
    GLOBAL_COUNTS_PLAYERS_KEY,
    GLOBAL_COUNTS_CLANS_KEY,
    GLOBAL_COUNTS_MAPS_KEY,
    GLOBAL_COUNTS_GAME_TYPES_KEY,
    GLOBAL_COUNTS_GAME_SERVERS_KEY
  ]);

  return {
    players: Number(playersStr || '0'),
    clans: Number(clansStr || '0'),
    maps: Number(mapsStr || '0'),
    gameTypes: Number(gameTypesStr || '0'),
    gameServers: Number(gameServersStr || '0'),
  };
}

export async function getGlobalCountsLastId(redis: Redis) {
  const [
    playersLastId,
    clansLastId,
    gameTypesLastId,
    mapsLastId,
    gameServersLastId
  ] = await redis.mget([
    GLOBAL_COUNTS_PLAYERS_LAST_ID_KEY,
    GLOBAL_COUNTS_CLANS_LAST_ID_KEY,
    GLOBAL_COUNTS_GAME_TYPES_LAST_ID_KEY,
    GLOBAL_COUNTS_MAPS_LAST_ID_KEY,
    GLOBAL_COUNTS_GAME_SERVERS_LAST_ID_KEY
  ])

  return {
    playersLastId: Number(playersLastId || '0'),
    clansLastId: Number(clansLastId || '0'),
    gameTypesLastId: Number(gameTypesLastId || '0'),
    mapsLastId: Number(mapsLastId || '0'),
    gameServersLastId: Number(gameServersLastId || '0'),
  };
}

export async function incrementGlobalPlayerCount(redis: Redis, increment: number, lastId: number) {
  const pipeline = redis.multi();
  pipeline.incrby(GLOBAL_COUNTS_PLAYERS_KEY, increment);
  pipeline.set(GLOBAL_COUNTS_PLAYERS_LAST_ID_KEY, lastId.toString());
  await pipeline.exec();
}

export async function incrementGlobalClanCount(redis: Redis, increment: number, lastId: number) {
  const pipeline = redis.multi();
  pipeline.incrby(GLOBAL_COUNTS_CLANS_KEY, increment);
  pipeline.set(GLOBAL_COUNTS_CLANS_LAST_ID_KEY, lastId.toString());
  await pipeline.exec();
}

export async function incrementGlobalGameTypeCount(redis: Redis, increment: number, lastId: number) {
  const pipeline = redis.multi();
  pipeline.incrby(GLOBAL_COUNTS_GAME_TYPES_KEY, increment);
  pipeline.set(GLOBAL_COUNTS_GAME_TYPES_LAST_ID_KEY, lastId.toString());
  await pipeline.exec();
}

export async function incrementGlobalMapCount(redis: Redis, increment: number, lastId: number) {
  const pipeline = redis.multi();
  pipeline.incrby(GLOBAL_COUNTS_MAPS_KEY, increment);
  pipeline.set(GLOBAL_COUNTS_MAPS_LAST_ID_KEY, lastId.toString());
  await pipeline.exec();
}

export async function incrementGlobalGameServerCount(redis: Redis, increment: number, lastId: number) {
  const pipeline = redis.multi();
  pipeline.incrby(GLOBAL_COUNTS_GAME_SERVERS_KEY, increment);
  pipeline.set(GLOBAL_COUNTS_GAME_SERVERS_LAST_ID_KEY, lastId.toString());
  await pipeline.exec();
}
