import { updateCount } from "./updateGlobalCounts";
import redis from "redis-mock";

describe("updateCounts", () => {
  test("create a new count", async () => {
    const redisClient = redis.createClient();

    await updateCount({
      redisClient,
      getNewEntities: async () => [{ createdAt: new Date() }],
      lastUpdatedAtKey: "test-0-lastUpdatedAt",
      countKey: "test-0-count",
    });

    expect(await redisClient.get("test-0-count")).toBe("1");
    expect(await redisClient.get("test-0-lastUpdatedAt")).toBe(new Date().toISOString());
  });

  test("update an existing count", async () => {
    const redisClient = redis.createClient();

    await redisClient.set("test-1-count", "1");
    await redisClient.set("test-1-lastUpdatedAt", new Date().toISOString());

    await updateCount({
      redisClient,
      getNewEntities: async () => [{ createdAt: new Date() }],
      lastUpdatedAtKey: "test-1-lastUpdatedAt",
      countKey: "test-1-count",
    });

    expect(await redisClient.get("test-1-count")).toBe("2");
    expect(await redisClient.get("test-1-lastUpdatedAt")).toBe(new Date().toISOString());
  });
});
