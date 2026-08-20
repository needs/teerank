import { addMinutes } from "date-fns";
import { prismaMock } from "../../test/mockPrisma";
import { DayAggregator, OBSERVATION_SECONDS, RollupSnapshot } from "../rollup/aggregateDay";
import { rollupDay } from "./rollupDay";

const day = new Date('2026-08-19T00:00:00.000Z');

const newClient = (playerName: string, clanName: string | null = null, inGame = true) => ({
  playerName,
  clanName,
  inGame,
});

const newSnapshot = (
  minutes: number,
  clients: RollupSnapshot['clients'],
  overrides: Partial<RollupSnapshot> = {}
): RollupSnapshot => ({
  createdAt: addMinutes(day, minutes),
  gameServerId: 1,
  mapId: 1,
  gameTypeName: 'CTF',
  numClients: clients.length,
  clients,
  ...overrides,
});

describe('DayAggregator', () => {
  test('one in-game player with a clan', () => {
    const aggregator = new DayAggregator();
    aggregator.addSnapshot(newSnapshot(0, [newClient('player0', 'clan0')]));

    const rollup = aggregator.finalize();

    expect(rollup.players).toEqual([{ playerName: 'player0', playTime: OBSERVATION_SECONDS }]);
    expect(rollup.serverDays).toEqual([{ gameServerId: 1, avgClients: 1, maxClients: 1 }]);
    expect(rollup.maps).toEqual([{ mapId: 1, playTime: OBSERVATION_SECONDS, playerCount: 1 }]);
    expect(rollup.gameTypes).toEqual([
      { gameTypeName: 'CTF', playTime: OBSERVATION_SECONDS, playerCount: 1 },
    ]);
    expect(rollup.clans).toEqual([
      { clanName: 'clan0', playTime: OBSERVATION_SECONDS, playerCount: 1 },
    ]);
  });

  test('spectators count for presence but not playtime', () => {
    const aggregator = new DayAggregator();
    aggregator.addSnapshot(newSnapshot(0, [newClient('player0', 'clan0', false)]));

    const rollup = aggregator.finalize();

    expect(rollup.players).toEqual([{ playerName: 'player0', playTime: 0 }]);
    expect(rollup.maps[0]).toEqual({ mapId: 1, playTime: 0, playerCount: 1 });
    expect(rollup.clans[0]).toEqual({ clanName: 'clan0', playTime: 0, playerCount: 1 });
    // A spectator is still a connected client.
    expect(rollup.serverDays[0].avgClients).toBe(1);
  });

  test('duplicated client names are counted once', () => {
    const aggregator = new DayAggregator();
    aggregator.addSnapshot(newSnapshot(0, [newClient('player0'), newClient('player0')]));

    const rollup = aggregator.finalize();

    expect(rollup.players).toEqual([{ playerName: 'player0', playTime: OBSERVATION_SECONDS }]);
  });

  test('servers with only empty snapshots are skipped', () => {
    const aggregator = new DayAggregator();
    aggregator.addSnapshot(newSnapshot(0, []));
    aggregator.addSnapshot(newSnapshot(0, [newClient('player0')], { gameServerId: 2 }));

    const rollup = aggregator.finalize();

    expect(rollup.serverDays).toEqual([{ gameServerId: 2, avgClients: 1, maxClients: 1 }]);
    expect(rollup.maps).toEqual([{ mapId: 1, playTime: OBSERVATION_SECONDS, playerCount: 1 }]);
  });

  test('empty hours do not dilute the daily average', () => {
    const aggregator = new DayAggregator();
    aggregator.addSnapshot(newSnapshot(0, [newClient('player0'), newClient('player1')]));
    aggregator.addSnapshot(newSnapshot(5, [newClient('player0'), newClient('player1')]));
    aggregator.addSnapshot(newSnapshot(60, []));
    aggregator.addSnapshot(newSnapshot(120, [newClient('player0'), newClient('player1'), newClient('player2'), newClient('player3')]));

    const rollup = aggregator.finalize();

    // Hour 0 averages 2, hour 1 is empty and ignored, hour 2 averages 4.
    expect(rollup.serverDays).toEqual([{ gameServerId: 1, avgClients: 3, maxClients: 4 }]);
  });

  test('distinct players across snapshots and gametypes', () => {
    const aggregator = new DayAggregator();
    aggregator.addSnapshot(newSnapshot(0, [newClient('player0', 'clan0')]));
    aggregator.addSnapshot(
      newSnapshot(5, [newClient('player0', 'clan0'), newClient('player1')], {
        gameServerId: 2,
        mapId: 2,
        gameTypeName: 'DM',
      })
    );

    const rollup = aggregator.finalize();

    expect(rollup.players).toEqual([
      { playerName: 'player0', playTime: 2 * OBSERVATION_SECONDS },
      { playerName: 'player1', playTime: OBSERVATION_SECONDS },
    ]);
    expect(rollup.gameTypes).toEqual([
      { gameTypeName: 'CTF', playTime: OBSERVATION_SECONDS, playerCount: 1 },
      { gameTypeName: 'DM', playTime: 2 * OBSERVATION_SECONDS, playerCount: 2 },
    ]);
    expect(rollup.clans).toEqual([
      { clanName: 'clan0', playTime: 2 * OBSERVATION_SECONDS, playerCount: 1 },
    ]);
  });
});

describe('rollupDay', () => {
  const mockLookups = () => {
    prismaMock.player.findMany.mockResolvedValue([{ name: 'player0', id: 11 }] as never);
    prismaMock.clan.findMany.mockResolvedValue([{ name: 'clan0', id: 21 }] as never);
    prismaMock.gameType.findMany.mockResolvedValue([{ name: 'CTF', id: 31 }] as never);
    prismaMock.$queryRawTyped.mockResolvedValue([] as never);
    prismaMock.$transaction.mockImplementation(((callback: (tx: unknown) => unknown) =>
      callback(prismaMock)) as never);
  };

  test('skips a day that is already rolled up', async () => {
    prismaMock.playerDay.findFirst.mockResolvedValue({ playerId: 1 } as never);

    await rollupDay({ day: '2026-08-19' });

    expect(prismaMock.gameServerSnapshot.findMany).not.toHaveBeenCalled();
    expect(prismaMock.$transaction).not.toHaveBeenCalled();
  });

  test('skips a day that is not over', async () => {
    const today = new Date().toISOString().slice(0, 10);

    await rollupDay({ day: today });

    expect(prismaMock.playerDay.findFirst).not.toHaveBeenCalled();
    expect(prismaMock.gameServerSnapshot.findMany).not.toHaveBeenCalled();
  });

  test('aggregates a day and writes all five tables', async () => {
    prismaMock.playerDay.findFirst.mockResolvedValue(null);
    prismaMock.gameServerSnapshot.findMany.mockResolvedValue([
      {
        id: 1,
        createdAt: addMinutes(day, 30),
        gameServerId: 1,
        mapId: 1,
        numClients: 1,
        map: { gameTypeName: 'CTF' },
        clients: [{ playerName: 'player0', clanName: 'clan0', inGame: true }],
      },
    ] as never);
    mockLookups();

    await rollupDay({ day: '2026-08-19' });

    expect(prismaMock.playerDay.deleteMany).toHaveBeenCalledWith({ where: { day } });
    expect(prismaMock.serverDay.deleteMany).toHaveBeenCalledWith({ where: { day } });

    expect(prismaMock.playerDay.createMany).toHaveBeenCalledWith({
      data: [{ day, playerId: 11, playTime: OBSERVATION_SECONDS }],
    });
    expect(prismaMock.serverDay.createMany).toHaveBeenCalledWith({
      data: [{ day, gameServerId: 1, avgClients: 1, maxClients: 1 }],
    });
    expect(prismaMock.mapDay.createMany).toHaveBeenCalledWith({
      data: [{ day, mapId: 1, playTime: OBSERVATION_SECONDS, playerCount: 1 }],
    });
    expect(prismaMock.gameTypeDay.createMany).toHaveBeenCalledWith({
      data: [{ day, gameTypeId: 31, playTime: OBSERVATION_SECONDS, playerCount: 1 }],
    });
    expect(prismaMock.clanDay.createMany).toHaveBeenCalledWith({
      data: [{ day, clanId: 21, playTime: OBSERVATION_SECONDS, playerCount: 1 }],
    });
  });

  test('players that no longer exist are dropped', async () => {
    prismaMock.playerDay.findFirst.mockResolvedValue(null);
    prismaMock.gameServerSnapshot.findMany.mockResolvedValue([
      {
        id: 1,
        createdAt: addMinutes(day, 30),
        gameServerId: 1,
        mapId: 1,
        numClients: 2,
        map: { gameTypeName: 'CTF' },
        clients: [
          { playerName: 'player0', clanName: 'clan0', inGame: true },
          { playerName: 'deleted', clanName: null, inGame: true },
        ],
      },
    ] as never);
    mockLookups();

    await rollupDay({ day: '2026-08-19' });

    const call = prismaMock.playerDay.createMany.mock.calls[0][0] as {
      data: { playerId: number }[];
    };
    expect(call.data).toHaveLength(1);
    expect(call.data[0].playerId).toBe(11);
  });
});
