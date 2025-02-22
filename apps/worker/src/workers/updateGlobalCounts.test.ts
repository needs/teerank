import { updateClansCount, updateGameServersCount, updateGameTypesCount, updateMapsCount, updatePlayersCount } from "./updateGlobalCounts";
import { prismaMock } from "../../test/mockPrisma";
import { Clan, GameServer, GameType, Map, Player, RankMethod } from "@prisma/client";
import { getGlobalCounts } from "@teerank/teerank";
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

test("updatePlayersCount", async () => {
  prismaMock.player.findMany.mockResolvedValue([mockPlayer]);

  await updatePlayersCount(new Date(0));
  await updatePlayersCount(new Date(0));

  const globalCounts = await getGlobalCounts(redis);
  expect(globalCounts.players).toBe(2);
});

test("updateClansCount", async () => {
  prismaMock.clan.findMany.mockResolvedValue([mockClan]);

  await updateClansCount(new Date(0));
  await updateClansCount(new Date(0));

  const globalCounts = await getGlobalCounts(redis);
  expect(globalCounts.clans).toBe(2);
});

test("updateGameServersCount", async () => {
  prismaMock.gameServer.findMany.mockResolvedValue([mockGameServer]);

  await updateGameServersCount(new Date(0));
  await updateGameServersCount(new Date(0));

  const globalCounts = await getGlobalCounts(redis);
  expect(globalCounts.gameServers).toBe(2);
});

test("updateMapsCount", async () => {
  prismaMock.map.findMany.mockResolvedValue([mockMap]);

  await updateMapsCount(new Date(0));
  await updateMapsCount(new Date(0));

  const globalCounts = await getGlobalCounts(redis);
  expect(globalCounts.maps).toBe(2);
});

test("updateGameTypesCount", async () => {
  prismaMock.gameType.findMany.mockResolvedValue([mockGameType]);

  await updateGameTypesCount(new Date(0));
  await updateGameTypesCount(new Date(0));

  const globalCounts = await getGlobalCounts(redis);
  expect(globalCounts.gameTypes).toBe(2);
});
