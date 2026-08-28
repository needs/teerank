import { minutesToMilliseconds } from "date-fns";
import { listDdnetMapsMissingThumb } from "@prisma/client/sql";
import { getEnvInt, wait } from "@teerank/teerank";
import { prisma } from "../prisma";
import { DdnetMapInfoRow, DdnetMapRow, fetchMapThumbnail } from "./stream";
import { resolveMapIds } from "./applyChunk";

const DDNET_THUMBNAILS_PER_RUN = getEnvInt('DDNET_THUMBNAILS_PER_RUN', 300);
const DDNET_THUMBNAIL_PAUSE_MS = getEnvInt('DDNET_THUMBNAIL_PAUSE_MS', 200);

export function parseMapperNames(mapper: string) {
  return [...new Set(
    mapper.split(/,|&/).map((name) => name.trim()).filter((name) => name.length > 0)
  )];
}

export async function importMapMetadata(mapRows: DdnetMapRow[], mapInfoRows: DdnetMapInfoRow[]) {
  const mapIds = await resolveMapIds(mapRows.map((row) => row.name));
  const infoByName = new Map(mapInfoRows.map((row) => [row.name, row]));

  const previousPoints = new Map(
    (await prisma.ddnetMap.findMany({ select: { mapId: true, points: true } }))
      .map((row) => [row.mapId, row.points])
  );

  const pointsChangedMapIds: number[] = [];

  await prisma.$transaction(
    async (tx) => {
      for (const row of mapRows) {
        const mapId = mapIds.get(row.name)!;
        const info = infoByName.get(row.name);

        const metadata = {
          category: row.category,
          points: row.points,
          stars: row.stars,
          mapper: row.mapper,
          releasedAt: row.releasedAt,
          width: info?.width ?? null,
          height: info?.height ?? null,
          tiles: info?.tiles ?? [],
        };

        await tx.ddnetMap.upsert({
          where: { mapId },
          create: { mapId, ...metadata },
          update: metadata,
        });

        if (previousPoints.get(mapId) !== row.points) {
          pointsChangedMapIds.push(mapId);
        }

        const mapperNames = parseMapperNames(row.mapper);
        await tx.ddnetMapMapper.deleteMany({ where: { mapId } });
        await tx.ddnetMapMapper.createMany({
          data: mapperNames.map((mapperName) => ({ mapperName, mapId })),
          skipDuplicates: true,
        });
      }
    },
    { timeout: minutesToMilliseconds(5), maxWait: minutesToMilliseconds(1) }
  );

  return { pointsChangedMapIds };
}

export async function syncMapThumbnails() {
  const missing = await prisma.$queryRawTyped(
    listDdnetMapsMissingThumb(DDNET_THUMBNAILS_PER_RUN)
  );

  let fetched = 0;

  for (const map of missing) {
    const result = await fetchMapThumbnail(map.name, null);

    if (result.status === 'fetched') {
      await prisma.ddnetMapThumb.upsert({
        where: { mapId: map.mapId },
        create: { mapId: map.mapId, png: result.png, etag: result.etag, fetchedAt: new Date() },
        update: { png: result.png, etag: result.etag, fetchedAt: new Date() },
      });
      fetched += 1;
    }

    await wait(DDNET_THUMBNAIL_PAUSE_MS);
  }

  if (missing.length > 0) {
    console.log(`DDNet thumbnails: fetched ${fetched}/${missing.length} missing`);
  }
}
