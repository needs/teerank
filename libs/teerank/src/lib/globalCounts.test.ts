import { getGlobalCounts, getGlobalCountsLastUpdatedAt } from "./globalCounts";
import Redis from "ioredis-mock";
import { incrementGlobalClanCount, incrementGlobalGameServerCount, incrementGlobalGameTypeCount, incrementGlobalMapCount, incrementGlobalPlayerCount } from "./globalCounts";
import { REDIS_HOST, REDIS_PORT } from './redisConfig';

const redis = new Redis(REDIS_PORT, REDIS_HOST);

beforeEach(async () => {
  await redis.flushall();
});

test("returns the lowest possible date the first time", async () => {
  const globalCountsLastUpdatedAt = await getGlobalCountsLastUpdatedAt(redis);

  expect(globalCountsLastUpdatedAt.playersLastUpdatedAt).toEqual(new Date(0));
  expect(globalCountsLastUpdatedAt.clansLastUpdatedAt).toEqual(new Date(0));
  expect(globalCountsLastUpdatedAt.mapsLastUpdatedAt).toEqual(new Date(0));
  expect(globalCountsLastUpdatedAt.gameTypesLastUpdatedAt).toEqual(new Date(0));
  expect(globalCountsLastUpdatedAt.gameServersLastUpdatedAt).toEqual(new Date(0));
});

test("should return the last updated at date for each count", async () => {
  const lastUpdatedAtPlayers = new Date();
  const lastUpdatedAtClans = new Date();
  const lastUpdatedAtMaps = new Date();
  const lastUpdatedAtGameTypes = new Date();
  const lastUpdatedAtGameServers = new Date();

  await incrementGlobalPlayerCount(redis, 1, lastUpdatedAtPlayers);
  await incrementGlobalClanCount(redis, 1, lastUpdatedAtClans);
  await incrementGlobalMapCount(redis, 1, lastUpdatedAtMaps);
  await incrementGlobalGameTypeCount(redis, 1, lastUpdatedAtGameTypes);
  await incrementGlobalGameServerCount(redis, 1, lastUpdatedAtGameServers);

  const globalCountsLastUpdatedAt = await getGlobalCountsLastUpdatedAt(redis);

  expect(globalCountsLastUpdatedAt.playersLastUpdatedAt).toEqual(lastUpdatedAtPlayers);
  expect(globalCountsLastUpdatedAt.clansLastUpdatedAt).toEqual(lastUpdatedAtClans);
  expect(globalCountsLastUpdatedAt.mapsLastUpdatedAt).toEqual(lastUpdatedAtMaps);
  expect(globalCountsLastUpdatedAt.gameTypesLastUpdatedAt).toEqual(lastUpdatedAtGameTypes);
  expect(globalCountsLastUpdatedAt.gameServersLastUpdatedAt).toEqual(lastUpdatedAtGameServers);

  const globalCounts = await getGlobalCounts(redis);

  expect(globalCounts.players).toEqual(1);
  expect(globalCounts.clans).toEqual(1);
  expect(globalCounts.maps).toEqual(1);
  expect(globalCounts.gameTypes).toEqual(1);
  expect(globalCounts.gameServers).toEqual(1);
});
