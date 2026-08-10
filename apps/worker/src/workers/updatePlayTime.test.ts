import { addMinutes } from "date-fns";
import { prismaMock } from "../../test/mockPrisma";
import { updatePlayTime } from "./updatePlayTime";
import { GameServerSnapshot, GameServerClient } from "@prisma/client";

const newClient = (playerName: string, clanName?: string): GameServerClient => ({
  playerName,
  clanName: clanName ?? null,
  score: 0,
  inGame: true,
  country: 0,
  snapshotId: 1,
  id: 1,
});

const newSnapshot = (id: number, createdAt: Date, clients: GameServerClient[]): Partial<GameServerSnapshot> & {
  id: number,
  map: { gameTypeName: string },
  clients: GameServerClient[]
} => ({
  id,
  createdAt,
  name: 'snapshot',
  version: 'version',
  maxClients: clients.length,
  numClients: clients.length,
  maxPlayers: clients.length,
  numPlayers: clients.length,
  gameServerId: 1,
  mapId: 1,
  map: {
    gameTypeName: 'gameType'
  },
  clients
});

const mockFirstSnapshot = (baseDate: Date, clients: GameServerClient[]) => {
  const snapshot = newSnapshot(1, baseDate, clients);
  prismaMock.gameServerSnapshot.findFirst.mockResolvedValue(snapshot as GameServerSnapshot);
  return snapshot;
};

const mockSecondSnapshot = (baseDate: Date, clients: GameServerClient[]) => {
  const snapshot = newSnapshot(2, baseDate, clients);
  prismaMock.gameServerSnapshot.findUniqueOrThrow.mockResolvedValue(snapshot as GameServerSnapshot);
  return snapshot;
};

// Each table is written with a single multi-row statement; find it by its
// SQL fragment and return the interpolated parameter values.
function rawStatementValues(fragment: string): unknown[] | null {
  const call = prismaMock.$executeRaw.mock.calls.find(
    (args) => (args[0] as unknown as ReadonlyArray<string>).join('?').includes(fragment)
  );

  if (call === undefined) {
    return null;
  }

  return (call[1] as { values: unknown[] }).values;
}

function checkPlayTimes(expectedPlayerPlayTimes: number[], expectedClanPlayTimes: number[], expectedClanPlayerPlayTimes: number[], expectedGlobalPlayTime: number) {
  const playerInfoMapValues = rawStatementValues('INSERT INTO "PlayerInfoMap"');
  const playerInfoGameTypeValues = rawStatementValues('INSERT INTO "PlayerInfoGameType"');
  const playerValues = rawStatementValues('UPDATE "Player" SET');

  expectedPlayerPlayTimes.forEach((playTime, index) => {
    // (playerName, mapId, playTime) per row
    expect(playerInfoMapValues).toEqual(expect.arrayContaining([`player${index}`, 1, playTime]));
    // (playerName, gameTypeName, playTime) per row
    expect(playerInfoGameTypeValues).toEqual(expect.arrayContaining([`player${index}`, 'gameType', playTime]));
    // (playerName, playTime) per row
    expect(playerValues).toEqual(expect.arrayContaining([`player${index}`, playTime]));
  });

  const clanInfoMapValues = rawStatementValues('INSERT INTO "ClanInfoMap"');
  const clanInfoGameTypeValues = rawStatementValues('INSERT INTO "ClanInfoGameType"');
  const clanValues = rawStatementValues('UPDATE "Clan" SET');

  expectedClanPlayTimes.forEach((playTime, index) => {
    expect(clanInfoMapValues).toEqual(expect.arrayContaining([`clan${index}`, 1, playTime]));
    expect(clanInfoGameTypeValues).toEqual(expect.arrayContaining([`clan${index}`, 'gameType', playTime]));
    expect(clanValues).toEqual(expect.arrayContaining([`clan${index}`, playTime]));
  });

  const clanPlayerInfoValues = rawStatementValues('INSERT INTO "ClanPlayerInfo"');

  expectedClanPlayerPlayTimes.forEach((playTime) => {
    expect(clanPlayerInfoValues).toEqual(expect.arrayContaining([playTime]));
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
  const clients = [newClient('player0', 'clan0')];
  const snapshot = mockSecondSnapshot(baseDate, clients);
  prismaMock.gameServerSnapshot.findFirst.mockResolvedValue(null);

  await updatePlayTime({ snapshotId: snapshot.id });

  // A zero delta writes nothing at all.
  expect(prismaMock.$executeRaw).not.toHaveBeenCalled();
  expect(prismaMock.gameType.update).not.toHaveBeenCalled();
  expect(prismaMock.map.update).not.toHaveBeenCalled();
  expect(prismaMock.gameServer.update).not.toHaveBeenCalled();
});

test('One player, no clan', async () => {
  const baseDate = new Date();
  const clients = [newClient('player0')];

  mockFirstSnapshot(baseDate, clients);
  mockSecondSnapshot(addMinutes(baseDate, 5), clients);

  await updatePlayTime({ snapshotId: 2 });
  checkPlayTimes([5 * 60], [], [], 5 * 60);

  // No clan statements at all when no client has a clan.
  expect(rawStatementValues('INSERT INTO "ClanInfoMap"')).toBeNull();
  expect(rawStatementValues('UPDATE "Clan" SET')).toBeNull();
  expect(rawStatementValues('INSERT INTO "ClanPlayerInfo"')).toBeNull();
});

test('One player, one clan', async () => {
  const baseDate = new Date();
  const clients = [newClient('player0', 'clan0')];

  mockFirstSnapshot(baseDate, clients);
  mockSecondSnapshot(addMinutes(baseDate, 5), clients);

  await updatePlayTime({ snapshotId: 2 });
  checkPlayTimes([5 * 60], [5 * 60], [5 * 60], 5 * 60);
});

test('Two players, same clan', async () => {
  const baseDate = new Date();
  const clients = [
    newClient('player0', 'clan0'),
    newClient('player1', 'clan0')
  ];

  mockFirstSnapshot(baseDate, clients);
  mockSecondSnapshot(addMinutes(baseDate, 5), clients);

  await updatePlayTime({ snapshotId: 2 });
  checkPlayTimes([5 * 60, 5 * 60], [10 * 60], [5 * 60, 5 * 60], 10 * 60);
});

test('Two players, different clan', async () => {
  const baseDate = new Date();
  const clients = [
    newClient('player0', 'clan0'),
    newClient('player1', 'clan1')
  ];

  mockFirstSnapshot(baseDate, clients);
  mockSecondSnapshot(addMinutes(baseDate, 5), clients);

  await updatePlayTime({ snapshotId: 2 });
  checkPlayTimes([5 * 60, 5 * 60], [5 * 60, 5 * 60], [5 * 60, 5 * 60], 2 * 5 * 60);
});
