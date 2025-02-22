import { addMinutes } from "date-fns";
import { prismaMock } from "../../test/mockPrisma";
import { updatePlayTime } from "./updatePlayTime";
import { toBigIntArray } from "../utils";
import { GameServerSnapshot, Player, Clan, Map, GameType, GameServer, PlayerInfoMap, PlayerInfoGameType, ClanInfoMap, ClanInfoGameType, ClanPlayerInfo, GameServerClient } from "@prisma/client";

const mockSnapshot = (id: number, createdAt: Date, numPlayers: number): Partial<GameServerSnapshot> & {
  id: number,
  map: { gameTypeName: string },
  clients: GameServerClient[]
} => ({
  id,
  createdAt,
  name: 'snapshot',
  version: 'version',
  maxClients: numPlayers,
  numClients: numPlayers,
  maxPlayers: numPlayers,
  numPlayers: numPlayers,
  gameServerId: 1,
  mapId: 1,
  map: {
    gameTypeName: 'gameType'
  },
  clients: []
});

const mockClient = (playerName: string, clanName?: string): GameServerClient => ({
  playerName,
  clanName: clanName ?? null,
  score: 0,
  inGame: true,
  country: 0,
  snapshotId: 1,
  id: 1,
});

function mockPlayTimeUpdates() {
  // Mock player updates
  prismaMock.player.update.mockResolvedValue({ playTime: BigInt(0) } as Player);

  // Mock player info upserts
  prismaMock.playerInfoMap.upsert.mockResolvedValue({ playTime: BigInt(0) } as PlayerInfoMap);
  prismaMock.playerInfoGameType.upsert.mockResolvedValue({ playTime: BigInt(0) } as PlayerInfoGameType);

  // Mock clan updates and upserts
  prismaMock.clan.update.mockResolvedValue({ playTime: BigInt(0) } as Clan);
  prismaMock.clanInfoMap.upsert.mockResolvedValue({ playTime: BigInt(0) } as ClanInfoMap);
  prismaMock.clanInfoGameType.upsert.mockResolvedValue({ playTime: BigInt(0) } as ClanInfoGameType);
  prismaMock.clanPlayerInfo.upsert.mockResolvedValue({ playTime: BigInt(0) } as ClanPlayerInfo);

  // Mock global updates
  prismaMock.gameType.update.mockResolvedValue({ playTime: BigInt(0) } as GameType);
  prismaMock.map.update.mockResolvedValue({ playTime: BigInt(0) } as Map);
  prismaMock.gameServer.update.mockResolvedValue({ playTime: BigInt(0) } as GameServer);
}

async function checkPlayTimes(expectedPlayerPlayTimes: number[], expectedClanPlayTimes: number[], expectedClanPlayerPlayTimes: number[], expectedGlobalPlayTime: number) {
  // Check player updates
  expectedPlayerPlayTimes.forEach((playTime, index) => {
    expect(prismaMock.player.update).toHaveBeenCalledWith({
      where: { name: `player${index}` },
      data: { playTime: { increment: playTime } }
    });

    expect(prismaMock.playerInfoMap.upsert).toHaveBeenCalledWith(expect.objectContaining({
      where: {
        playerName_mapId: {
          mapId: 1,
          playerName: `player${index}`
        }
      },
      update: { playTime: { increment: playTime } }
    }));

    expect(prismaMock.playerInfoGameType.upsert).toHaveBeenCalledWith(expect.objectContaining({
      where: {
        playerName_gameTypeName: {
          gameTypeName: 'gameType',
          playerName: `player${index}`
        }
      },
      update: { playTime: { increment: playTime } }
    }));
  });

  // Check clan updates
  expectedClanPlayTimes.forEach((playTime, index) => {
    expect(prismaMock.clan.update).toHaveBeenCalledWith({
      where: { name: `clan${index}` },
      data: { playTime: { increment: playTime } }
    });

    expect(prismaMock.clanInfoMap.upsert).toHaveBeenCalledWith(expect.objectContaining({
      where: {
        clanName_mapId: {
          mapId: 1,
          clanName: `clan${index}`
        }
      },
      update: { playTime: { increment: playTime } }
    }));

    expect(prismaMock.clanInfoGameType.upsert).toHaveBeenCalledWith(expect.objectContaining({
      where: {
        clanName_gameTypeName: {
          gameTypeName: 'gameType',
          clanName: `clan${index}`
        }
      },
      update: { playTime: { increment: playTime } }
    }));
  });

  // Check clan-player updates
  expectedClanPlayerPlayTimes.forEach((playTime, index) => {
    expect(prismaMock.clanPlayerInfo.upsert).toHaveBeenCalledWith(expect.objectContaining({
      where: {
        clanName_playerName: {
          clanName: `clan${Math.floor(index / 2)}`,
          playerName: `player${index}`
        }
      },
      update: { playTime: { increment: playTime } }
    }));
  });

  // Check global updates
  expect(prismaMock.gameType.update).toHaveBeenCalledWith({
    where: { name: 'gameType' },
    data: { playTime: { increment: expectedGlobalPlayTime } }
  });

  expect(prismaMock.map.update).toHaveBeenCalledWith({
    where: { id: 1 },
    data: { playTime: { increment: expectedGlobalPlayTime } }
  });

  expect(prismaMock.gameServer.update).toHaveBeenCalledWith({
    where: { id: 1 },
    data: { playTime: { increment: expectedGlobalPlayTime } }
  });
}

test('Single snapshot', async () => {
  const baseDate = new Date();
  const snapshot = mockSnapshot(1, baseDate, 1);
  snapshot.clients = [mockClient('player0', 'clan0')];

  prismaMock.gameServerSnapshot.findUniqueOrThrow.mockResolvedValue(snapshot as any);
  prismaMock.gameServerClient.findMany.mockResolvedValue(snapshot.clients);
  prismaMock.gameServerSnapshot.findFirst.mockResolvedValue(null);

  mockPlayTimeUpdates();

  await updatePlayTime(snapshot.id);
  await checkPlayTimes([0], [0], [0], 0);
});

test('One player, no clan', async () => {
  const baseDate = new Date();
  const snapshot1 = mockSnapshot(1, baseDate, 1);
  const snapshot2 = mockSnapshot(2, addMinutes(baseDate, 5), 1);

  snapshot1.clients = [mockClient('player0')];
  snapshot2.clients = [mockClient('player0')];

  prismaMock.gameServerSnapshot.findUniqueOrThrow.mockResolvedValue(snapshot2 as any);
  prismaMock.gameServerClient.findMany.mockResolvedValue(snapshot2.clients);
  prismaMock.gameServerSnapshot.findFirst.mockResolvedValue(snapshot1 as any);

  // Mock the play time checks
  prismaMock.player.findMany.mockResolvedValue([{ playTime: BigInt(5 * 60) } as Player]);
  prismaMock.playerInfoMap.findMany.mockResolvedValue([{ playTime: BigInt(5 * 60) } as PlayerInfoMap]);
  prismaMock.playerInfoGameType.findMany.mockResolvedValue([{ playTime: BigInt(5 * 60) } as PlayerInfoGameType]);
  prismaMock.clan.findMany.mockResolvedValue([]);
  prismaMock.clanInfoMap.findMany.mockResolvedValue([]);
  prismaMock.clanInfoGameType.findMany.mockResolvedValue([]);
  prismaMock.clanPlayerInfo.findMany.mockResolvedValue([]);
  prismaMock.gameType.findUniqueOrThrow.mockResolvedValue({ playTime: BigInt(5 * 60) } as GameType);
  prismaMock.map.findUniqueOrThrow.mockResolvedValue({ playTime: BigInt(5 * 60) } as Map);
  prismaMock.gameServer.findUniqueOrThrow.mockResolvedValue({ playTime: BigInt(5 * 60) } as GameServer);

  mockPlayTimeUpdates();

  await updatePlayTime(snapshot2.id);
  await checkPlayTimes([5 * 60], [], [], 5 * 60);
});

test('One player, one clan', async () => {
  const baseDate = new Date();
  const snapshot1 = mockSnapshot(1, baseDate, 1);
  const snapshot2 = mockSnapshot(2, addMinutes(baseDate, 5), 1);

  snapshot1.clients = [mockClient('player0', 'clan0')];
  snapshot2.clients = [mockClient('player0', 'clan0')];

  prismaMock.gameServerSnapshot.findUniqueOrThrow.mockResolvedValue(snapshot2 as any);
  prismaMock.gameServerClient.findMany.mockResolvedValue(snapshot2.clients);
  prismaMock.gameServerSnapshot.findFirst.mockResolvedValue(snapshot1 as any);

  // Mock the play time checks
  prismaMock.player.findMany.mockResolvedValue([{ playTime: BigInt(5 * 60) } as Player]);
  prismaMock.playerInfoMap.findMany.mockResolvedValue([{ playTime: BigInt(5 * 60) } as PlayerInfoMap]);
  prismaMock.playerInfoGameType.findMany.mockResolvedValue([{ playTime: BigInt(5 * 60) } as PlayerInfoGameType]);
  prismaMock.clan.findMany.mockResolvedValue([{ playTime: BigInt(5 * 60) } as Clan]);
  prismaMock.clanInfoMap.findMany.mockResolvedValue([{ playTime: BigInt(5 * 60) } as ClanInfoMap]);
  prismaMock.clanInfoGameType.findMany.mockResolvedValue([{ playTime: BigInt(5 * 60) } as ClanInfoGameType]);
  prismaMock.clanPlayerInfo.findMany.mockResolvedValue([{ playTime: BigInt(5 * 60) } as ClanPlayerInfo]);
  prismaMock.gameType.findUniqueOrThrow.mockResolvedValue({ playTime: BigInt(5 * 60) } as GameType);
  prismaMock.map.findUniqueOrThrow.mockResolvedValue({ playTime: BigInt(5 * 60) } as Map);
  prismaMock.gameServer.findUniqueOrThrow.mockResolvedValue({ playTime: BigInt(5 * 60) } as GameServer);

  mockPlayTimeUpdates();

  await updatePlayTime(snapshot2.id);
  await checkPlayTimes([5 * 60], [5 * 60], [5 * 60], 5 * 60);
});

test('Two players, same clan', async () => {
  const baseDate = new Date();
  const snapshot1 = mockSnapshot(1, baseDate, 2);
  const snapshot2 = mockSnapshot(2, addMinutes(baseDate, 5), 2);

  snapshot1.clients = [
    mockClient('player0', 'clan0'),
    mockClient('player1', 'clan0')
  ];
  snapshot2.clients = [
    mockClient('player0', 'clan0'),
    mockClient('player1', 'clan0')
  ];

  prismaMock.gameServerSnapshot.findUniqueOrThrow.mockResolvedValue(snapshot2 as any);
  prismaMock.gameServerClient.findMany.mockResolvedValue(snapshot2.clients);
  prismaMock.gameServerSnapshot.findFirst.mockResolvedValue(snapshot1 as any);

  mockPlayTimeUpdates();

  await updatePlayTime(snapshot2.id);

  // Modify checkPlayTimes to handle the case where multiple players are in the same clan
  const playerPlayTime = 5 * 60;
  const clanPlayTime = 2 * playerPlayTime; // Total clan play time is sum of both players

  // Check individual player updates
  expect(prismaMock.player.update).toHaveBeenCalledWith({
    where: { name: 'player0' },
    data: { playTime: { increment: playerPlayTime } }
  });
  expect(prismaMock.player.update).toHaveBeenCalledWith({
    where: { name: 'player1' },
    data: { playTime: { increment: playerPlayTime } }
  });

  // Check clan update - should be called once with total play time
  expect(prismaMock.clan.update).toHaveBeenCalledWith({
    where: { name: 'clan0' },
    data: { playTime: { increment: clanPlayTime } }
  });

  // Check global updates
  expect(prismaMock.gameType.update).toHaveBeenCalledWith({
    where: { name: 'gameType' },
    data: { playTime: { increment: clanPlayTime } }
  });
  expect(prismaMock.map.update).toHaveBeenCalledWith({
    where: { id: 1 },
    data: { playTime: { increment: clanPlayTime } }
  });
  expect(prismaMock.gameServer.update).toHaveBeenCalledWith({
    where: { id: 1 },
    data: { playTime: { increment: clanPlayTime } }
  });
});

test('Two players, different clan', async () => {
  const baseDate = new Date();
  const snapshot1 = mockSnapshot(1, baseDate, 2);
  const snapshot2 = mockSnapshot(2, addMinutes(baseDate, 5), 2);

  snapshot1.clients = [
    mockClient('player0', 'clan0'),
    mockClient('player1', 'clan1')
  ];
  snapshot2.clients = [
    mockClient('player0', 'clan0'),
    mockClient('player1', 'clan1')
  ];

  prismaMock.gameServerSnapshot.findUniqueOrThrow.mockResolvedValue(snapshot2 as any);
  prismaMock.gameServerClient.findMany.mockResolvedValue(snapshot2.clients);
  prismaMock.gameServerSnapshot.findFirst.mockResolvedValue(snapshot1 as any);

  // Mock the play time checks with appropriate values
  prismaMock.player.findMany.mockResolvedValue([
    { playTime: BigInt(5 * 60) } as Player,
    { playTime: BigInt(5 * 60) } as Player
  ]);
  prismaMock.playerInfoMap.findMany.mockResolvedValue([
    { playTime: BigInt(5 * 60) } as PlayerInfoMap,
    { playTime: BigInt(5 * 60) } as PlayerInfoMap
  ]);
  prismaMock.playerInfoGameType.findMany.mockResolvedValue([
    { playTime: BigInt(5 * 60) } as PlayerInfoGameType,
    { playTime: BigInt(5 * 60) } as PlayerInfoGameType
  ]);
  prismaMock.clan.findMany.mockResolvedValue([
    { playTime: BigInt(5 * 60) } as Clan,
    { playTime: BigInt(5 * 60) } as Clan
  ]);
  prismaMock.clanInfoMap.findMany.mockResolvedValue([
    { playTime: BigInt(5 * 60) } as ClanInfoMap,
    { playTime: BigInt(5 * 60) } as ClanInfoMap
  ]);
  prismaMock.clanInfoGameType.findMany.mockResolvedValue([
    { playTime: BigInt(5 * 60) } as ClanInfoGameType,
    { playTime: BigInt(5 * 60) } as ClanInfoGameType
  ]);
  prismaMock.clanPlayerInfo.findMany.mockResolvedValue([
    { playTime: BigInt(5 * 60) } as ClanPlayerInfo,
    { playTime: BigInt(5 * 60) } as ClanPlayerInfo
  ]);
  prismaMock.gameType.findUniqueOrThrow.mockResolvedValue({ playTime: BigInt(5 * 60) } as GameType);
  prismaMock.map.findUniqueOrThrow.mockResolvedValue({ playTime: BigInt(5 * 60) } as Map);
  prismaMock.gameServer.findUniqueOrThrow.mockResolvedValue({ playTime: BigInt(5 * 60) } as GameServer);

  mockPlayTimeUpdates();

  await updatePlayTime(snapshot2.id);
  await checkPlayTimes([5 * 60, 5 * 60], [5 * 60, 5 * 60], [5 * 60, 5 * 60], 2 * 5 * 60);
});
