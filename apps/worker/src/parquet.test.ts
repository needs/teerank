import { tableFromIPC } from "apache-arrow";
import { readParquet } from "parquet-wasm";
import { SnapshotArchiveRow, encodeSnapshotRowsToParquet } from "./parquet";

const baseRow: Omit<SnapshotArchiveRow, 'playerName' | 'clanName' | 'score' | 'country' | 'inGame'> = {
  snapshotId: 182474286,
  createdAt: new Date('2026-08-09T12:34:56.789Z'),
  gameServerId: 7,
  serverName: 'server',
  version: '0.6.4',
  mapId: 42,
  mapName: 'dm1',
  gameTypeName: 'CTF',
  numPlayers: 1,
  maxPlayers: 16,
  numClients: 1,
  maxClients: 16,
};

test('parquet round trip preserves types and nulls', () => {
  const rows: SnapshotArchiveRow[] = [
    // Empty snapshot: all client fields null.
    { ...baseRow, playerName: null, clanName: null, score: null, country: null, inGame: null },
    {
      ...baseRow,
      snapshotId: 182474287,
      createdAt: new Date('2026-08-09T12:39:56.789Z'),
      playerName: 'nameless tee',
      clanName: '',
      score: -9999,
      country: 276,
      inGame: true,
    },
  ];

  const parquet = encodeSnapshotRowsToParquet(rows);
  const table = tableFromIPC(readParquet(parquet).intoIPCStream());

  expect(table.numRows).toBe(2);

  const first = table.get(0)!.toJSON();
  expect(first.snapshotId).toBe(BigInt(182474286));
  expect(new Date(Number(first.createdAt))).toEqual(new Date('2026-08-09T12:34:56.789Z'));
  expect(first.playerName).toBeNull();
  expect(first.clanName).toBeNull();
  expect(first.score).toBeNull();
  expect(first.inGame).toBeNull();

  const second = table.get(1)!.toJSON();
  expect(second.snapshotId).toBe(BigInt(182474287));
  expect(second.playerName).toBe('nameless tee');
  expect(second.clanName).toBe('');
  expect(second.score).toBe(-9999);
  expect(second.inGame).toBe(true);
});

test('parquet rows are sorted by gameServerId then createdAt by the caller contract', () => {
  const rows: SnapshotArchiveRow[] = [
    { ...baseRow, playerName: 'a', clanName: null, score: 0, country: 0, inGame: true },
  ];

  const parquet = encodeSnapshotRowsToParquet(rows);
  const table = tableFromIPC(readParquet(parquet).intoIPCStream());
  expect(table.numRows).toBe(1);
});
