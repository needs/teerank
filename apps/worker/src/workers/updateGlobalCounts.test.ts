import { updateClansCount, updateGameServersCount, updateGameTypesCount, updateMapsCount, updatePlayersCount } from "./updateGlobalCounts";
import { prismaMock } from "../../test/mockPrisma";
import { Clan, GameServer, GameType, Map, Player, RankMethod } from "@prisma/client";
import { getGlobalCounts, getGlobalCountsLastUpdatedAt } from "@teerank/teerank";
import { redis } from "../redis";

const mockPlayer: Player = {
  createdAt: new Date(),
  name: "test",
  updatedAt: new Date(),
  lastSeenAt: new Date(),
  clanName: "test",
  playTime: BigInt(1),
}

const mockClan: Clan = {
  createdAt: new Date(),
  name: "test",
  updatedAt: new Date(),
  playTime: BigInt(1),
  activePlayerCount: 1,
}

const mockMap: Map = {
  createdAt: new Date(),
  name: "test",
  playTime: BigInt(1),
  playerCount: 1,
  clanCount: 1,
  gameServerCount: 1,
  id: 1,
  gameTypeName: "test",
}

const mockGameType: GameType = {
  createdAt: new Date(),
  name: "test",
  playTime: BigInt(1),
  playerCount: 1,
  clanCount: 1,
  gameServerCount: 1,
  rankMethod: RankMethod.ELO,
  mapCount: 1,
}

const mockGameServer: GameServer = {
  createdAt: new Date(),
  updatedAt: new Date(),
  lastSeenAt: new Date(),
  playTime: BigInt(1),
  failureCount: 1,
  ip: "127.0.0.1",
  port: 1,
  masterServerId: 1,
  id: 1,
}

beforeEach(async () => {
  await redis.flushall();
});

describe("updatePlayersCount", () => {
  test("single player", async () => {
    prismaMock.player.findMany.mockResolvedValue([mockPlayer]);

    await updatePlayersCount(new Date(0));

    const globalCounts = await getGlobalCounts(redis);
    expect(globalCounts.players).toBe(1);

    const globalCountsLastUpdatedAt = await getGlobalCountsLastUpdatedAt(redis);
    expect(globalCountsLastUpdatedAt.playersLastUpdatedAt).toEqual(mockPlayer.createdAt);
  });

  test("no players", async () => {
    prismaMock.player.findMany.mockResolvedValue([]);

    await updatePlayersCount(new Date(0));

    const globalCounts = await getGlobalCounts(redis);
    expect(globalCounts.players).toBe(0);

    const globalCountsLastUpdatedAt = await getGlobalCountsLastUpdatedAt(redis);
    expect(globalCountsLastUpdatedAt.playersLastUpdatedAt).toEqual(new Date(0));
  });
});

describe("updateClansCount", () => {
  test("single clan", async () => {
    prismaMock.clan.findMany.mockResolvedValue([mockClan]);

    await updateClansCount(new Date(0));

    const globalCounts = await getGlobalCounts(redis);
    expect(globalCounts.clans).toBe(1);

    const globalCountsLastUpdatedAt = await getGlobalCountsLastUpdatedAt(redis);
    expect(globalCountsLastUpdatedAt.clansLastUpdatedAt).toEqual(mockClan.createdAt);
  });

  test("no clans", async () => {
    prismaMock.clan.findMany.mockResolvedValue([]);

    await updateClansCount(new Date(0));

    const globalCounts = await getGlobalCounts(redis);
    expect(globalCounts.clans).toBe(0);

    const globalCountsLastUpdatedAt = await getGlobalCountsLastUpdatedAt(redis);
    expect(globalCountsLastUpdatedAt.clansLastUpdatedAt).toEqual(new Date(0));
  });
});

describe("updateGameServersCount", () => {
  test("single game server", async () => {
    prismaMock.gameServer.findMany.mockResolvedValue([mockGameServer]);

    await updateGameServersCount(0);

    const globalCounts = await getGlobalCounts(redis);
    expect(globalCounts.gameServers).toBe(1);

    const globalCountsLastUpdatedAt = await getGlobalCountsLastUpdatedAt(redis);
    expect(globalCountsLastUpdatedAt.gameServersLastUpdatedId).toEqual(mockGameServer.id);
  });

  test("no game servers", async () => {
    prismaMock.gameServer.findMany.mockResolvedValue([]);

    await updateGameServersCount(0);

    const globalCounts = await getGlobalCounts(redis);
    expect(globalCounts.gameServers).toBe(0);

    const globalCountsLastUpdatedAt = await getGlobalCountsLastUpdatedAt(redis);
    expect(globalCountsLastUpdatedAt.gameServersLastUpdatedId).toEqual(0);
  });
});

describe("updateMapsCount", () => {
  test("single map", async () => {
    prismaMock.map.findMany.mockResolvedValue([mockMap]);

    await updateMapsCount(0);

    const globalCounts = await getGlobalCounts(redis);
    expect(globalCounts.maps).toBe(1);

    const globalCountsLastUpdatedAt = await getGlobalCountsLastUpdatedAt(redis);
    expect(globalCountsLastUpdatedAt.mapsLastUpdatedId).toEqual(mockMap.id);
  });

  test("no maps", async () => {
    prismaMock.map.findMany.mockResolvedValue([]);

    await updateMapsCount(0);

    const globalCounts = await getGlobalCounts(redis);
    expect(globalCounts.maps).toBe(0);

    const globalCountsLastUpdatedAt = await getGlobalCountsLastUpdatedAt(redis);
    expect(globalCountsLastUpdatedAt.mapsLastUpdatedId).toEqual(0);
  });
});

describe("updateGameTypesCount", () => {
  test("single game type", async () => {
    prismaMock.gameType.findMany.mockResolvedValue([mockGameType]);

    await updateGameTypesCount(new Date(0));

    const globalCounts = await getGlobalCounts(redis);
    expect(globalCounts.gameTypes).toBe(1);

    const globalCountsLastUpdatedAt = await getGlobalCountsLastUpdatedAt(redis);
    expect(globalCountsLastUpdatedAt.gameTypesLastUpdatedAt).toEqual(mockGameType.createdAt);
  });

  test("no game types", async () => {
    prismaMock.gameType.findMany.mockResolvedValue([]);

    await updateGameTypesCount(new Date(0));

    const globalCounts = await getGlobalCounts(redis);
    expect(globalCounts.gameTypes).toBe(0);

    const globalCountsLastUpdatedAt = await getGlobalCountsLastUpdatedAt(redis);
    expect(globalCountsLastUpdatedAt.gameTypesLastUpdatedAt).toEqual(new Date(0));
  });
});
