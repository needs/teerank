import { Redis } from "ioredis";

const GLOBAL_COUNTS_PLAYERS_LAST_UPDATED_AT_KEY = 'global-counts-players-last-updated-at';
const GLOBAL_COUNTS_CLANS_LAST_UPDATED_AT_KEY = 'global-counts-clans-last-updated-at';
const GLOBAL_COUNTS_MAPS_LAST_UPDATED_AT_KEY = 'global-counts-maps-last-updated-at';
const GLOBAL_COUNTS_GAME_TYPES_LAST_UPDATED_AT_KEY = 'global-counts-game-types-last-updated-at';
const GLOBAL_COUNTS_GAME_SERVERS_LAST_UPDATED_AT_KEY = 'global-counts-game-servers-last-updated-at';

const GLOBAL_COUNTS_PLAYERS_KEY = 'global-counts-players';
const GLOBAL_COUNTS_CLANS_KEY = 'global-counts-clans';
const GLOBAL_COUNTS_MAPS_KEY = 'global-counts-maps';
const GLOBAL_COUNTS_GAME_TYPES_KEY = 'global-counts-game-types';
const GLOBAL_COUNTS_GAME_SERVERS_KEY = 'global-counts-game-servers';

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

export async function getGlobalCountsLastUpdatedAt(redis: Redis) {
  const [
    playersLastUpdatedAt,
    clansLastUpdatedAt,
    mapsLastUpdatedAt,
    gameTypesLastUpdatedAt,
    gameServersLastUpdatedAt
  ] = await redis.mget([
    GLOBAL_COUNTS_PLAYERS_LAST_UPDATED_AT_KEY,
    GLOBAL_COUNTS_CLANS_LAST_UPDATED_AT_KEY,
    GLOBAL_COUNTS_MAPS_LAST_UPDATED_AT_KEY,
    GLOBAL_COUNTS_GAME_TYPES_LAST_UPDATED_AT_KEY,
    GLOBAL_COUNTS_GAME_SERVERS_LAST_UPDATED_AT_KEY
  ]);

  return {
    playersLastUpdatedAt: new Date(Number(playersLastUpdatedAt || '0')),
    clansLastUpdatedAt: new Date(Number(clansLastUpdatedAt || '0')),
    mapsLastUpdatedAt: new Date(Number(mapsLastUpdatedAt || '0')),
    gameTypesLastUpdatedAt: new Date(Number(gameTypesLastUpdatedAt || '0')),
    gameServersLastUpdatedAt: new Date(Number(gameServersLastUpdatedAt || '0')),
  };
}

export async function incrementGlobalPlayerCount(redis: Redis, increment: number, lastUpdatedAt: Date) {
  const pipeline = redis.multi();
  pipeline.incrby(GLOBAL_COUNTS_PLAYERS_KEY, increment);
  pipeline.set(GLOBAL_COUNTS_PLAYERS_LAST_UPDATED_AT_KEY, lastUpdatedAt.getTime().toString());
  await pipeline.exec();
}

export async function incrementGlobalClanCount(redis: Redis, increment: number, lastUpdatedAt: Date) {
  const pipeline = redis.multi();
  pipeline.incrby(GLOBAL_COUNTS_CLANS_KEY, increment);
  pipeline.set(GLOBAL_COUNTS_CLANS_LAST_UPDATED_AT_KEY, lastUpdatedAt.getTime().toString());
  await pipeline.exec();
}

export async function incrementGlobalMapCount(redis: Redis, increment: number, lastUpdatedAt: Date) {
  const pipeline = redis.multi();
  pipeline.incrby(GLOBAL_COUNTS_MAPS_KEY, increment);
  pipeline.set(GLOBAL_COUNTS_MAPS_LAST_UPDATED_AT_KEY, lastUpdatedAt.getTime().toString());
  await pipeline.exec();
}

export async function incrementGlobalGameTypeCount(redis: Redis, increment: number, lastUpdatedAt: Date) {
  const pipeline = redis.multi();
  pipeline.incrby(GLOBAL_COUNTS_GAME_TYPES_KEY, increment);
  pipeline.set(GLOBAL_COUNTS_GAME_TYPES_LAST_UPDATED_AT_KEY, lastUpdatedAt.getTime().toString());
  await pipeline.exec();
}

export async function incrementGlobalGameServerCount(redis: Redis, increment: number, lastUpdatedAt: Date) {
  const pipeline = redis.multi();
  pipeline.incrby(GLOBAL_COUNTS_GAME_SERVERS_KEY, increment);
  pipeline.set(GLOBAL_COUNTS_GAME_SERVERS_LAST_UPDATED_AT_KEY, lastUpdatedAt.getTime().toString());
  await pipeline.exec();
}
