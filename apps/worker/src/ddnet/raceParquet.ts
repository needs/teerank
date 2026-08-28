import {
  Float32,
  Float64,
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
import { DdnetRaceRow, DdnetTeamRaceRow } from "./stream";

const SPLIT_COUNT = 25;

function writeTable(columns: Record<string, unknown>) {
  const table = new Table(columns as never);
  const wasmTable = WasmTable.fromIPCStream(tableToIPC(table, 'stream'));
  const writerProperties = new WriterPropertiesBuilder()
    .setCompression(Compression.ZSTD)
    .build();

  return writeParquet(wasmTable, writerProperties);
}

export function encodeRaceRowsToParquet(rows: DdnetRaceRow[]): Uint8Array {
  const columns: Record<string, unknown> = {
    mapName: vectorFromArray(rows.map((row) => row.mapName), new Utf8()),
    playerName: vectorFromArray(rows.map((row) => row.playerName), new Utf8()),
    time: vectorFromArray(rows.map((row) => row.time), new Float64()),
    timestamp: vectorFromArray(rows.map((row) => row.timestamp.getTime()), new TimestampMillisecond()),
  };

  for (let index = 0; index < SPLIT_COUNT; index++) {
    columns[`cp${index + 1}`] = vectorFromArray(
      rows.map((row) => row.splits[index] ?? 0),
      new Float32()
    );
  }

  return writeTable(columns);
}

export function decodeRaceRowsFromParquet(parquet: Uint8Array): DdnetRaceRow[] {
  const table = tableFromIPC(readParquet(parquet).intoIPCStream());
  const rows: DdnetRaceRow[] = [];

  for (let index = 0; index < table.numRows; index++) {
    const row = table.get(index)!.toJSON() as Record<string, unknown>;

    const splits: number[] = [];
    let hasData = false;
    for (let cp = 1; cp <= SPLIT_COUNT; cp++) {
      const value = Number(row[`cp${cp}`] ?? 0);
      splits.push(value);
      if (value > 0) {
        hasData = true;
      }
    }

    rows.push({
      mapName: String(row.mapName),
      playerName: String(row.playerName),
      time: Number(row.time),
      timestamp: new Date(Number(row.timestamp)),
      splits: hasData ? splits : [],
    });
  }

  return rows;
}

export function encodeTeamRaceRowsToParquet(rows: DdnetTeamRaceRow[]): Uint8Array {
  return writeTable({
    mapName: vectorFromArray(rows.map((row) => row.mapName), new Utf8()),
    playerName: vectorFromArray(rows.map((row) => row.playerName), new Utf8()),
    time: vectorFromArray(rows.map((row) => row.time), new Float64()),
    teamId: vectorFromArray(rows.map((row) => row.teamId), new Utf8()),
    timestamp: vectorFromArray(rows.map((row) => row.timestamp.getTime()), new TimestampMillisecond()),
  });
}

export function decodeTeamRaceRowsFromParquet(parquet: Uint8Array): DdnetTeamRaceRow[] {
  const table = tableFromIPC(readParquet(parquet).intoIPCStream());
  const rows: DdnetTeamRaceRow[] = [];

  for (let index = 0; index < table.numRows; index++) {
    const row = table.get(index)!.toJSON() as Record<string, unknown>;

    rows.push({
      mapName: String(row.mapName),
      playerName: String(row.playerName),
      time: Number(row.time),
      teamId: String(row.teamId),
      timestamp: new Date(Number(row.timestamp)),
    });
  }

  return rows;
}
