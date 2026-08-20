import {
  Bool,
  Int32,
  Int64,
  Table,
  TimestampMillisecond,
  Utf8,
  tableFromIPC,
  tableToIPC,
  vectorFromArray,
} from "apache-arrow";
import {
  Compression,
  Table as WasmTable,
  WriterPropertiesBuilder,
  readParquet,
  writeParquet,
} from "parquet-wasm";

// One flat row per client-observation. playerName is null when the snapshot
// had zero clients, so empty snapshots are still recorded. mapName and
// gameTypeName are denormalised into every row to keep the archive
// self-contained — they dictionary-encode to almost nothing under Parquet.
export type SnapshotArchiveRow = {
  snapshotId: number;
  createdAt: Date;
  gameServerId: number;
  serverName: string;
  version: string;
  mapId: number;
  mapName: string;
  gameTypeName: string;
  numPlayers: number;
  maxPlayers: number;
  numClients: number;
  maxClients: number;
  playerName: string | null;
  clanName: string | null;
  score: number | null;
  country: number | null;
  inGame: boolean | null;
};

export function encodeSnapshotRowsToParquet(rows: SnapshotArchiveRow[]): Uint8Array {
  const table = new Table({
    snapshotId: vectorFromArray(rows.map((row) => BigInt(row.snapshotId)), new Int64()),
    createdAt: vectorFromArray(rows.map((row) => row.createdAt.getTime()), new TimestampMillisecond()),
    gameServerId: vectorFromArray(rows.map((row) => row.gameServerId), new Int32()),
    serverName: vectorFromArray(rows.map((row) => row.serverName), new Utf8()),
    version: vectorFromArray(rows.map((row) => row.version), new Utf8()),
    mapId: vectorFromArray(rows.map((row) => row.mapId), new Int32()),
    mapName: vectorFromArray(rows.map((row) => row.mapName), new Utf8()),
    gameTypeName: vectorFromArray(rows.map((row) => row.gameTypeName), new Utf8()),
    numPlayers: vectorFromArray(rows.map((row) => row.numPlayers), new Int32()),
    maxPlayers: vectorFromArray(rows.map((row) => row.maxPlayers), new Int32()),
    numClients: vectorFromArray(rows.map((row) => row.numClients), new Int32()),
    maxClients: vectorFromArray(rows.map((row) => row.maxClients), new Int32()),
    playerName: vectorFromArray(rows.map((row) => row.playerName), new Utf8()),
    clanName: vectorFromArray(rows.map((row) => row.clanName), new Utf8()),
    score: vectorFromArray(rows.map((row) => row.score), new Int32()),
    country: vectorFromArray(rows.map((row) => row.country), new Int32()),
    inGame: vectorFromArray(rows.map((row) => row.inGame), new Bool()),
  });

  const wasmTable = WasmTable.fromIPCStream(tableToIPC(table, 'stream'));
  const writerProperties = new WriterPropertiesBuilder()
    .setCompression(Compression.ZSTD)
    .build();

  return writeParquet(wasmTable, writerProperties);
}

export function decodeSnapshotRowsFromParquet(parquet: Uint8Array): SnapshotArchiveRow[] {
  const table = tableFromIPC(readParquet(parquet).intoIPCStream());
  const rows: SnapshotArchiveRow[] = [];

  for (let index = 0; index < table.numRows; index++) {
    const row = table.get(index)!.toJSON();

    rows.push({
      snapshotId: Number(row.snapshotId),
      createdAt: new Date(Number(row.createdAt)),
      gameServerId: row.gameServerId,
      serverName: row.serverName,
      version: row.version,
      mapId: row.mapId,
      mapName: row.mapName,
      gameTypeName: row.gameTypeName,
      numPlayers: row.numPlayers,
      maxPlayers: row.maxPlayers,
      numClients: row.numClients,
      maxClients: row.maxClients,
      playerName: row.playerName,
      clanName: row.clanName,
      score: row.score,
      country: row.country,
      inGame: row.inGame,
    });
  }

  return rows;
}
