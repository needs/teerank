import { format } from 'date-fns';
import prisma from './prisma';

const TIER_ORDER = ['Novice', 'Moderate', 'Brutal', 'Insane'];

// @db.Date values come back as UTC midnight; re-anchor before formatting.
export function formatUtcDate(date: Date, pattern = 'MMM d, yyyy') {
  return format(
    new Date(date.getUTCFullYear(), date.getUTCMonth(), date.getUTCDate()),
    pattern
  );
}

export function getDdnetPlayer(playerId: number) {
  return prisma.ddnetPlayer.findUnique({
    where: { playerId },
  });
}

export type DdnetJourneyTier = {
  category: string;
  firstMapName: string;
  firstFinishAt: Date;
  finishCount: number;
  points: number;
};

export type DdnetJourney = {
  tiers: DdnetJourneyTier[];
  firstMapName: string | null;
  lastMapName: string | null;
};

export async function getDdnetJourney(
  playerId: number,
  lastFinishAt: Date
): Promise<DdnetJourney> {
  const bests = await prisma.ddnetRaceBest.findMany({
    where: { playerId },
    select: { mapId: true, finishCount: true, firstFinishAt: true, bestAt: true },
    orderBy: [{ firstFinishAt: 'asc' }, { mapId: 'asc' }],
  });

  const mapIds = bests.map((best) => best.mapId);

  const [maps, ddnetMaps] = await Promise.all([
    prisma.map.findMany({
      where: { id: { in: mapIds } },
      select: { id: true, name: true },
    }),
    prisma.ddnetMap.findMany({
      where: { mapId: { in: mapIds } },
      select: { mapId: true, category: true, points: true },
    }),
  ]);

  const mapNames = new Map(maps.map((map) => [map.id, map.name]));
  const ddnetMapsById = new Map(ddnetMaps.map((map) => [map.mapId, map]));

  const tiersByCategory = new Map<string, DdnetJourneyTier>();

  for (const best of bests) {
    const ddnetMap = ddnetMapsById.get(best.mapId);

    if (ddnetMap === undefined) {
      continue;
    }

    const tier = tiersByCategory.get(ddnetMap.category);

    if (tier === undefined) {
      tiersByCategory.set(ddnetMap.category, {
        category: ddnetMap.category,
        firstMapName: mapNames.get(best.mapId) ?? '',
        firstFinishAt: best.firstFinishAt,
        finishCount: best.finishCount,
        points: ddnetMap.points,
      });
    } else {
      tier.finishCount += best.finishCount;
      tier.points += ddnetMap.points;
    }
  }

  const tiers = [
    ...TIER_ORDER.flatMap((category) => tiersByCategory.get(category) ?? []),
    ...[...tiersByCategory.values()].filter(
      (tier) => !TIER_ORDER.includes(tier.category)
    ),
  ];

  const lastDay = lastFinishAt.toISOString().slice(0, 10);
  const lastBest = bests.find(
    (best) => best.bestAt.toISOString().slice(0, 10) === lastDay
  );

  return {
    tiers,
    firstMapName: bests.length > 0 ? mapNames.get(bests[0].mapId) ?? null : null,
    lastMapName: lastBest === undefined ? null : mapNames.get(lastBest.mapId) ?? null,
  };
}

export type DdnetBestTime = {
  mapId: number;
  mapName: string | null;
  category: string | null;
  time: number;
  rank: number;
  topPercent: number | null;
  bestAt: Date;
  hasSplits: boolean;
};

function topPercent(timeQuantiles: number[], time: number): number | null {
  if (timeQuantiles.length === 0) {
    return null;
  }

  const index = timeQuantiles.findIndex((quantile) => quantile >= time);

  return index === -1 ? 100 : Math.max(index, 1);
}

async function getRaceRanks(rows: { mapId: number; time: number }[]) {
  if (rows.length === 0) {
    return new Map<number, number>();
  }

  const ranks = await prisma.$queryRaw<{ mapId: number; rank: number }[]>`
    SELECT r."mapId", (COUNT(b."playerId") + 1)::int AS "rank"
    FROM unnest(
      ${rows.map((row) => row.mapId)}::int[],
      ${rows.map((row) => row.time)}::float8[]
    ) AS r("mapId", "time")
    LEFT JOIN "DdnetRaceBest" b
      ON b."mapId" = r."mapId" AND b."time" < r."time"
    GROUP BY r."mapId"
  `;

  return new Map(ranks.map((row) => [row.mapId, row.rank]));
}

export function countDdnetBestTimes(playerId: number) {
  return prisma.ddnetRaceBest.count({ where: { playerId } });
}

export async function getDdnetBestTimes(
  playerId: number,
  { skip, take }: { skip: number; take: number }
): Promise<DdnetBestTime[]> {
  const bests = await prisma.ddnetRaceBest.findMany({
    where: { playerId },
    orderBy: [{ bestAt: 'desc' }, { mapId: 'asc' }],
    skip,
    take,
  });

  const mapIds = bests.map((best) => best.mapId);

  const [maps, ddnetMaps, ranks] = await Promise.all([
    prisma.map.findMany({
      where: { id: { in: mapIds } },
      select: { id: true, name: true },
    }),
    prisma.ddnetMap.findMany({
      where: { mapId: { in: mapIds } },
      select: { mapId: true, category: true, timeQuantiles: true },
    }),
    getRaceRanks(bests),
  ]);

  const mapNames = new Map(maps.map((map) => [map.id, map.name]));
  const ddnetMapsById = new Map(ddnetMaps.map((map) => [map.mapId, map]));

  return bests.map((best) => {
    const ddnetMap = ddnetMapsById.get(best.mapId);

    return {
      mapId: best.mapId,
      mapName: mapNames.get(best.mapId) ?? null,
      category: ddnetMap?.category ?? null,
      time: best.time,
      rank: ranks.get(best.mapId) ?? 1,
      topPercent:
        ddnetMap === undefined ? null : topPercent(ddnetMap.timeQuantiles, best.time),
      bestAt: best.bestAt,
      hasSplits: best.splits.length > 0,
    };
  });
}

export type DdnetPartnerRow = {
  name: string;
  pollCount: number;
  occurrenceCount: number;
  finishCount: number;
  bestTime: number;
  bestMapName: string | null;
  lastFinishAt: Date;
};

export function countDdnetPartners(playerId: number) {
  return prisma.ddnetPartner.count({ where: { playerId } });
}

export async function getDdnetPartners(
  playerId: number,
  { skip, take }: { skip: number; take: number }
): Promise<DdnetPartnerRow[]> {
  const partners = await prisma.ddnetPartner.findMany({
    where: { playerId },
    orderBy: [{ finishCount: 'desc' }, { partnerId: 'asc' }],
    skip,
    take,
  });

  const [players, maps] = await Promise.all([
    prisma.player.findMany({
      where: { id: { in: partners.map((partner) => partner.partnerId) } },
      select: { id: true, name: true, pollCount: true, occurrenceCount: true },
    }),
    prisma.map.findMany({
      where: { id: { in: partners.map((partner) => partner.bestMapId) } },
      select: { id: true, name: true },
    }),
  ]);

  const playersById = new Map(players.map((player) => [player.id, player]));
  const mapNames = new Map(maps.map((map) => [map.id, map.name]));

  return partners.flatMap((partner) => {
    const player = playersById.get(partner.partnerId);

    if (player === undefined) {
      return [];
    }

    return {
      name: player.name,
      pollCount: player.pollCount,
      occurrenceCount: player.occurrenceCount,
      finishCount: partner.finishCount,
      bestTime: partner.bestTime,
      bestMapName: mapNames.get(partner.bestMapId) ?? null,
      lastFinishAt: partner.lastFinishAt,
    };
  });
}

export type DdnetPartnerMapRow = {
  mapId: number;
  mapName: string | null;
  bestTime: number;
  finishCount: number;
  duoRank: number;
  lastFinishAt: Date;
};

async function getDuoRanks(rows: { mapId: number; time: number }[]) {
  if (rows.length === 0) {
    return new Map<number, number>();
  }

  const ranks = await prisma.$queryRaw<{ mapId: number; rank: number }[]>`
    SELECT r."mapId", (COUNT(t."teamId") + 1)::int AS "rank"
    FROM unnest(
      ${rows.map((row) => row.mapId)}::int[],
      ${rows.map((row) => row.time)}::float8[]
    ) AS r("mapId", "time")
    LEFT JOIN "DdnetTeamRace" t
      ON t."mapId" = r."mapId" AND t."size" = 2 AND t."time" < r."time"
    GROUP BY r."mapId"
  `;

  return new Map(ranks.map((row) => [row.mapId, row.rank]));
}

export function countDdnetPartnerMaps(playerId: number, partnerId: number) {
  return prisma.ddnetPartnerMap.count({ where: { playerId, partnerId } });
}

export async function getDdnetPartnerMaps(
  playerId: number,
  partnerId: number,
  { skip, take }: { skip: number; take: number }
): Promise<DdnetPartnerMapRow[]> {
  const rows = await prisma.ddnetPartnerMap.findMany({
    where: { playerId, partnerId },
    orderBy: [{ lastFinishAt: 'desc' }, { mapId: 'asc' }],
    skip,
    take,
  });

  const [maps, duoRanks] = await Promise.all([
    prisma.map.findMany({
      where: { id: { in: rows.map((row) => row.mapId) } },
      select: { id: true, name: true },
    }),
    getDuoRanks(rows.map((row) => ({ mapId: row.mapId, time: row.bestTime }))),
  ]);

  const mapNames = new Map(maps.map((map) => [map.id, map.name]));

  return rows.map((row) => ({
    mapId: row.mapId,
    mapName: mapNames.get(row.mapId) ?? null,
    bestTime: row.bestTime,
    finishCount: row.finishCount,
    duoRank: duoRanks.get(row.mapId) ?? 1,
    lastFinishAt: row.lastFinishAt,
  }));
}
