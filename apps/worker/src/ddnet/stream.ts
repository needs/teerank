import { Readable } from "stream";
import { parse } from "csv-parse";
import * as unzipper from "unzipper";
import { getEnv } from "@teerank/teerank";

export const DDNET_STATS_URL = getEnv('DDNET_STATS_URL', 'https://ddnet.org/stats/ddnet-stats.zip');
export const DDNET_STATUS_BASE_URL = getEnv('DDNET_STATUS_BASE_URL', 'https://ddnet.org/status/csv');
export const DDNET_MAP_THUMB_BASE_URL = getEnv('DDNET_MAP_THUMB_BASE_URL', 'https://ddnet.org/ranks/maps');

export type DdnetRaceRow = {
  mapName: string;
  playerName: string;
  time: number;
  timestamp: Date;
  // 25 checkpoint times, or empty when the run recorded none.
  splits: number[];
};

export type DdnetTeamRaceRow = {
  mapName: string;
  playerName: string;
  time: number;
  teamId: string; // hex
  timestamp: Date;
};

export type DdnetMapRow = {
  name: string;
  category: string;
  points: number;
  stars: number;
  mapper: string;
  releasedAt: Date | null;
};

export type DdnetMapInfoRow = {
  name: string;
  width: number;
  height: number;
  tiles: string[];
};

export type DumpHandlers = {
  onMap?: (row: DdnetMapRow) => void | Promise<void>;
  onMapInfo?: (row: DdnetMapInfoRow) => void | Promise<void>;
  onRace?: (row: DdnetRaceRow) => void | Promise<void>;
  onTeamRace?: (row: DdnetTeamRaceRow) => void | Promise<void>;
};

// Dump timestamps have no zone marker; they are UTC on ddnet.org.
function parseDumpTimestamp(value: string): Date | null {
  if (value.startsWith('0000')) {
    return null;
  }

  const date = new Date(value.replace(' ', 'T') + 'Z');
  return Number.isNaN(date.getTime()) ? null : date;
}

function parseSplits(record: string[]): number[] {
  const splits: number[] = [];
  let hasData = false;

  for (let index = 5; index < 30; index++) {
    const value = Number(record[index] ?? 0);
    splits.push(value);
    if (value > 0) {
      hasData = true;
    }
  }

  return hasData ? splits : [];
}

export async function parseCsvEntry(
  entry: NodeJS.ReadableStream,
  onRecord: (record: string[], header: string[]) => void | Promise<void>
) {
  const parser = entry.pipe(parse({
    relax_column_count: true,
    bom: true,
    escape: '\\',
  }));

  let header: string[] | null = null;

  for await (const record of parser as AsyncIterable<string[]>) {
    if (header === null) {
      header = record;
      continue;
    }
    await onRecord(record, header);
  }
}

export async function fetchDumpLastModified() {
  const response = await fetch(DDNET_STATS_URL, { method: 'HEAD' });
  if (!response.ok) {
    throw new Error(`HEAD ${DDNET_STATS_URL} failed: ${response.status}`);
  }
  return response.headers.get('last-modified') ?? '';
}

// Streams every CSV inside the stats dump in one pass; entries arrive in
// archive order, so handlers must not assume maps come before races.
export async function streamStatsDump(handlers: DumpHandlers) {
  const response = await fetch(DDNET_STATS_URL);

  if (!response.ok || response.body === null) {
    throw new Error(`GET ${DDNET_STATS_URL} failed: ${response.status}`);
  }

  const source = Readable.fromWeb(response.body as never);
  const zip = source.pipe(unzipper.Parse({ forceStream: true }));

  try {
    for await (const entry of zip as AsyncIterable<unzipper.Entry>) {
      const fileName = entry.path.split('/').pop() ?? '';

      if (fileName === 'maps.csv' && handlers.onMap !== undefined) {
        await parseCsvEntry(entry, async (record) => {
          await handlers.onMap!({
            name: record[0],
            category: record[1],
            points: Number(record[2]),
            stars: Number(record[3]),
            mapper: record[4],
            releasedAt: parseDumpTimestamp(record[5]),
          });
        });
      } else if (fileName === 'mapinfo.csv' && handlers.onMapInfo !== undefined) {
        await parseCsvEntry(entry, async (record, header) => {
          const tiles: string[] = [];
          for (let index = 3; index < header.length; index++) {
            if (Number(record[index]) > 0) {
              tiles.push(header[index]);
            }
          }
          await handlers.onMapInfo!({
            name: record[0],
            width: Number(record[1]),
            height: Number(record[2]),
            tiles,
          });
        });
      } else if (fileName === 'race.csv' && handlers.onRace !== undefined) {
        await parseCsvEntry(entry, async (record) => {
          const timestamp = parseDumpTimestamp(record[3]);
          if (timestamp === null) {
            return;
          }
          await handlers.onRace!({
            mapName: record[0],
            playerName: record[1],
            time: Number(record[2]),
            timestamp,
            splits: parseSplits(record),
          });
        });
      } else if (fileName === 'teamrace.csv' && handlers.onTeamRace !== undefined) {
        await parseCsvEntry(entry, async (record) => {
          const timestamp = parseDumpTimestamp(record[4]);
          if (timestamp === null) {
            return;
          }
          await handlers.onTeamRace!({
            mapName: record[0],
            playerName: record[1],
            time: Number(record[2]),
            teamId: record[3].toLowerCase(),
            timestamp,
          });
        });
      } else {
        entry.autodrain();
      }
    }
  } finally {
    zip.destroy();
    source.destroy();
  }
}

// Sanitisation rule used by ddnet.org for map asset file names.
export function ddnetMapFileName(mapName: string) {
  return mapName.replace(/[^A-Za-z0-9]/g, '_');
}

export async function fetchMapThumbnail(mapName: string, etag: string | null) {
  const url = `${DDNET_MAP_THUMB_BASE_URL}/${encodeURIComponent(ddnetMapFileName(mapName))}.png`;
  const headers: Record<string, string> = {};

  if (etag !== null) {
    headers['If-None-Match'] = etag;
  }

  const response = await fetch(url, { headers });

  if (response.status === 304) {
    return { status: 'unchanged' as const };
  }
  if (response.status === 404) {
    return { status: 'missing' as const };
  }
  if (!response.ok) {
    throw new Error(`GET ${url} failed: ${response.status}`);
  }

  return {
    status: 'fetched' as const,
    png: Buffer.from(await response.arrayBuffer()),
    etag: response.headers.get('etag'),
  };
}
