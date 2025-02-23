import { getGlobalCounts, getGlobalCountsLastId } from "./globalCounts";
import Redis from "ioredis-mock";
import { incrementGlobalClanCount, incrementGlobalGameServerCount, incrementGlobalGameTypeCount, incrementGlobalMapCount, incrementGlobalPlayerCount } from "./globalCounts";
import { REDIS_HOST, REDIS_PORT } from './redisConfig';

const redis = new Redis(REDIS_PORT, REDIS_HOST);

beforeEach(async () => {
  await redis.flushall();
});

test("returns the lowest possible date the first time", async () => {
  const globalCountsLastUpdatedAt = await getGlobalCountsLastId(redis);

  expect(globalCountsLastUpdatedAt.playersLastId).toEqual(0);
  expect(globalCountsLastUpdatedAt.clansLastId).toEqual(0);
  expect(globalCountsLastUpdatedAt.gameTypesLastId).toEqual(0);
  expect(globalCountsLastUpdatedAt.mapsLastId).toEqual(0);
  expect(globalCountsLastUpdatedAt.gameServersLastId).toEqual(0);
});

test("should return the last updated at date for each count", async () => {
  const lastPlayerId = 1;
  const lastClanId = 1;
  const lastGameTypeId = 1;
  const lastMapId = 1;
  const lastGameServerId = 1;

  await incrementGlobalPlayerCount(redis, 1, lastPlayerId);
  await incrementGlobalClanCount(redis, 1, lastClanId);
  await incrementGlobalGameTypeCount(redis, 1, lastGameTypeId);
  await incrementGlobalMapCount(redis, 1, lastMapId);
  await incrementGlobalGameServerCount(redis, 1, lastGameServerId);

  const globalCountsLastUpdatedAt = await getGlobalCountsLastId(redis);

  expect(globalCountsLastUpdatedAt.playersLastId).toEqual(lastPlayerId);
  expect(globalCountsLastUpdatedAt.clansLastId).toEqual(lastClanId);
  expect(globalCountsLastUpdatedAt.gameTypesLastId).toEqual(lastGameTypeId);
  expect(globalCountsLastUpdatedAt.mapsLastId).toEqual(lastMapId);
  expect(globalCountsLastUpdatedAt.gameServersLastId).toEqual(lastGameServerId);

  const globalCounts = await getGlobalCounts(redis);

  expect(globalCounts.players).toEqual(1);
  expect(globalCounts.clans).toEqual(1);
  expect(globalCounts.maps).toEqual(1);
  expect(globalCounts.gameTypes).toEqual(1);
  expect(globalCounts.gameServers).toEqual(1);
});
