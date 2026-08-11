import { HeadObjectCommand, PutObjectCommand } from "@aws-sdk/client-s3";
import {
  ArchiveSnapshotsJobData,
  S3_BUCKET,
  getEnvInt,
  getS3Client,
  processArchiveSnapshotsJobs,
  wait,
} from "@teerank/teerank";
import { prisma } from "../prisma";
import { SnapshotArchiveRow, encodeSnapshotRowsToParquet } from "../parquet";

const SNAPSHOT_RETENTION_HOURS = getEnvInt('SNAPSHOT_RETENTION_HOURS', 48);
const ARCHIVE_BATCH_SIZE = getEnvInt('ARCHIVE_BATCH_SIZE', 5000);
const ARCHIVE_TIME_BUDGET_MS = getEnvInt('ARCHIVE_TIME_BUDGET_MS', 5 * 60 * 1000);
const ARCHIVE_BATCH_PAUSE_MS = getEnvInt('ARCHIVE_BATCH_PAUSE_MS', 200);

type ArchivableSnapshot = {
  id: number;
  createdAt: Date;
  gameServerId: number;
  name: string;
  version: string;
  mapId: number;
  map: { name: string; gameTypeName: string };
  numPlayers: number;
  maxPlayers: number;
  numClients: number;
  maxClients: number;
  clients: {
    playerName: string;
    clanName: string | null;
    score: number;
    country: number;
    inGame: boolean;
  }[];
};

function utcDay(date: Date) {
  return date.toISOString().slice(0, 10);
}

function padId(id: number) {
  return id.toString().padStart(10, '0');
}

function chunkByDay(snapshots: ArchivableSnapshot[]) {
  const chunks: { dt: string; snapshots: ArchivableSnapshot[] }[] = [];

  for (const snapshot of snapshots) {
    const dt = utcDay(snapshot.createdAt);
    const lastChunk = chunks[chunks.length - 1];

    if (lastChunk !== undefined && lastChunk.dt === dt) {
      lastChunk.snapshots.push(snapshot);
    } else {
      chunks.push({ dt, snapshots: [snapshot] });
    }
  }

  return chunks;
}

function flattenRows(snapshots: ArchivableSnapshot[]): SnapshotArchiveRow[] {
  const rows: SnapshotArchiveRow[] = [];

  for (const snapshot of snapshots) {
    const base = {
      snapshotId: snapshot.id,
      createdAt: snapshot.createdAt,
      gameServerId: snapshot.gameServerId,
      serverName: snapshot.name,
      version: snapshot.version,
      mapId: snapshot.mapId,
      mapName: snapshot.map.name,
      gameTypeName: snapshot.map.gameTypeName,
      numPlayers: snapshot.numPlayers,
      maxPlayers: snapshot.maxPlayers,
      numClients: snapshot.numClients,
      maxClients: snapshot.maxClients,
    };

    if (snapshot.clients.length === 0) {
      rows.push({ ...base, playerName: null, clanName: null, score: null, country: null, inGame: null });
    } else {
      for (const client of snapshot.clients) {
        rows.push({
          ...base,
          playerName: client.playerName,
          clanName: client.clanName,
          score: client.score,
          country: client.country,
          inGame: client.inGame,
        });
      }
    }
  }

  // Score recomputation reads consecutive snapshot pairs per server, so files
  // are sorted by (gameServerId, createdAt) — a sequential scan per server,
  // and row groups get tight min/max on gameServerId.
  rows.sort((a, b) =>
    a.gameServerId - b.gameServerId
    || a.createdAt.getTime() - b.createdAt.getTime()
    || a.snapshotId - b.snapshotId
  );

  return rows;
}

async function getWatermark() {
  const result = await prisma.$queryRaw<{ max: number | null }[]>`
    SELECT max(id)::int4 AS max FROM "GameServerSnapshot"
    WHERE "createdAt" < now() - make_interval(hours => ${SNAPSHOT_RETENTION_HOURS}::int)
  `;

  return result[0]?.max ?? null;
}

async function archiveBatch(snapshots: ArchivableSnapshot[]) {
  const s3 = getS3Client();

  for (const { dt, snapshots: chunk } of chunkByDay(snapshots)) {
    const firstId = chunk[0].id;
    const lastId = chunk[chunk.length - 1].id;
    const key = `snapshots/dt=${dt}/${padId(firstId)}-${padId(lastId)}.parquet`;
    const body = encodeSnapshotRowsToParquet(flattenRows(chunk));

    await s3.send(new PutObjectCommand({
      Bucket: S3_BUCKET,
      Key: key,
      Body: body,
      ContentType: 'application/vnd.apache.parquet',
    }));

    const head = await s3.send(new HeadObjectCommand({ Bucket: S3_BUCKET, Key: key }));
    if (!head.ContentLength) {
      throw new Error(`Archive object ${key} is missing or empty, not deleting rows`);
    }

    await prisma.gameServerClient.deleteMany({
      where: { snapshotId: { gte: firstId, lte: lastId } },
    });
    await prisma.gameServerSnapshot.deleteMany({
      where: { id: { gte: firstId, lte: lastId } },
    });

    console.log(`Archived ${chunk.length} snapshots to ${key}`);
  }
}

export async function archiveSnapshots(_data: ArchiveSnapshotsJobData) {
  const startedAt = Date.now();
  const watermark = await getWatermark();

  if (watermark === null) {
    return;
  }

  while (Date.now() - startedAt < ARCHIVE_TIME_BUDGET_MS) {
    const snapshots = await prisma.gameServerSnapshot.findMany({
      where: {
        id: { lte: watermark },
      },
      orderBy: {
        id: 'asc',
      },
      take: ARCHIVE_BATCH_SIZE,
      select: {
        id: true,
        createdAt: true,
        gameServerId: true,
        name: true,
        version: true,
        mapId: true,
        map: {
          select: {
            name: true,
            gameTypeName: true,
          },
        },
        numPlayers: true,
        maxPlayers: true,
        numClients: true,
        maxClients: true,
        clients: {
          select: {
            playerName: true,
            clanName: true,
            score: true,
            country: true,
            inGame: true,
          },
        },
      },
    });

    if (snapshots.length === 0) {
      break;
    }

    await archiveBatch(snapshots);

    if (snapshots.length < ARCHIVE_BATCH_SIZE) {
      break;
    }

    await wait(ARCHIVE_BATCH_PAUSE_MS);
  }
}

export async function startArchiveSnapshotsWorker() {
  return processArchiveSnapshotsJobs(archiveSnapshots);
}
