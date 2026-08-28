import { STUB_MIN_POLL_COUNT, STUB_OCCURRENCE_RATIO } from '@teerank/teerank';
import {
  countDdnetMapRecordsWithoutShared,
  listDdnetMapRecordsWithoutShared,
} from '@prisma/client/sql';
import prisma from './prisma';

export type RecordPlayer = {
  name: string;
  pollCount: number;
  occurrenceCount: number;
};

export async function getDdnetMap(mapId: number) {
  return prisma.ddnetMap.findUnique({
    where: {
      mapId,
    },
  });
}

export async function getDdnetMapHeader(mapId: number) {
  const ddnetMap = await getDdnetMap(mapId);

  if (ddnetMap === null) {
    return null;
  }

  const [mappers, thumb] = await Promise.all([
    prisma.ddnetMapMapper.findMany({
      where: {
        mapId,
      },
      orderBy: {
        mapperName: 'asc',
      },
    }),
    prisma.ddnetMapThumb.findUnique({
      select: {
        mapId: true,
      },
      where: {
        mapId,
      },
    }),
  ]);

  return {
    ddnetMap,
    mappers: mappers.map((mapper) => mapper.mapperName),
    hasThumb: thumb !== null,
  };
}

async function playersById(playerIds: number[]) {
  const players = await prisma.player.findMany({
    select: {
      id: true,
      name: true,
      pollCount: true,
      occurrenceCount: true,
    },
    where: {
      id: {
        in: [...new Set(playerIds)],
      },
    },
  });

  const byId = new Map(players.map((player) => [player.id, player]));

  return (playerId: number): RecordPlayer =>
    byId.get(playerId) ?? {
      name: `#${playerId}`,
      pollCount: 0,
      occurrenceCount: 0,
    };
}

export async function listSoloRecords({
  mapId,
  page,
  hideShared,
}: {
  mapId: number;
  page: number;
  hideShared: boolean;
}) {
  const take = 100;
  const skip = (page - 1) * take;

  if (hideShared) {
    const [rows, counts] = await Promise.all([
      prisma.$queryRawTyped(listDdnetMapRecordsWithoutShared(
        mapId, STUB_MIN_POLL_COUNT, STUB_OCCURRENCE_RATIO, take, skip
      )),
      prisma.$queryRawTyped(countDdnetMapRecordsWithoutShared(
        mapId, STUB_MIN_POLL_COUNT, STUB_OCCURRENCE_RATIO
      )),
    ]);

    return {
      count: Number(counts[0]?.count ?? 0),
      records: rows.map((row, index) => ({
        rank: skip + index + 1,
        players: [
          {
            name: row.name,
            pollCount: row.pollCount,
            occurrenceCount: row.occurrenceCount,
          },
        ],
        time: row.time,
        date: row.bestAt,
      })),
    };
  }

  const [bests, count] = await Promise.all([
    prisma.ddnetRaceBest.findMany({
      where: {
        mapId,
      },
      orderBy: [
        {
          time: 'asc',
        },
        {
          playerId: 'asc',
        },
      ],
      take,
      skip,
    }),
    prisma.ddnetRaceBest.count({
      where: {
        mapId,
      },
    }),
  ]);

  const playerFor = await playersById(bests.map((best) => best.playerId));

  return {
    count,
    records: bests.map((best, index) => ({
      rank: skip + index + 1,
      players: [playerFor(best.playerId)],
      time: best.time,
      date: best.bestAt,
    })),
  };
}

export async function listTeamRecords({
  mapId,
  page,
  size,
}: {
  mapId: number;
  page: number;
  size?: number;
}) {
  const take = 100;
  const skip = (page - 1) * take;
  const where = {
    mapId,
    size,
  };

  const [races, count] = await Promise.all([
    prisma.ddnetTeamRace.findMany({
      where,
      orderBy: [
        {
          time: 'asc',
        },
        {
          timestamp: 'asc',
        },
      ],
      take,
      skip,
    }),
    prisma.ddnetTeamRace.count({
      where,
    }),
  ]);

  const rosterRows = await prisma.ddnetTeamRacePlayer.findMany({
    where: {
      teamId: {
        in: races.map((race) => race.teamId),
      },
    },
  });

  const playerFor = await playersById(rosterRows.map((row) => row.playerId));

  const rosters = new Map<string, RecordPlayer[]>();

  for (const row of rosterRows) {
    const key = Buffer.from(row.teamId).toString('hex');
    const roster = rosters.get(key) ?? [];
    roster.push(playerFor(row.playerId));
    rosters.set(key, roster);
  }

  return {
    count,
    records: races.map((race, index) => ({
      rank: skip + index + 1,
      players: (
        rosters.get(Buffer.from(race.teamId).toString('hex')) ?? []
      ).sort((a, b) => a.name.localeCompare(b.name)),
      time: race.time,
      date: race.timestamp,
    })),
  };
}

export async function getRecordEvents(mapId: number) {
  const rows = await prisma.ddnetRecord.findMany({
    where: {
      mapId,
    },
    orderBy: [
      {
        timestamp: 'asc',
      },
      {
        playerId: 'asc',
      },
    ],
  });

  const playerFor = await playersById(rows.map((row) => row.playerId));

  const events: { date: Date; time: number; names: string[] }[] = [];

  for (const row of rows) {
    const last = events[events.length - 1];

    if (
      last !== undefined &&
      last.date.getTime() === row.timestamp.getTime() &&
      last.time === row.time
    ) {
      last.names.push(playerFor(row.playerId).name);
    } else {
      events.push({
        date: row.timestamp,
        time: row.time,
        names: [playerFor(row.playerId).name],
      });
    }
  }

  return events;
}

type FeedEvent = {
  mapId: number;
  date: Date;
  time: number;
  playerIds: number[];
};

function groupFeedEvents(
  rows: { mapId: number; timestamp: Date; playerId: number; time: number }[]
) {
  const events: FeedEvent[] = [];
  const byKey = new Map<string, FeedEvent>();

  for (const row of rows) {
    const key = `${row.mapId}\0${row.timestamp.getTime()}\0${row.time}`;
    const existing = byKey.get(key);

    if (existing !== undefined) {
      existing.playerIds.push(row.playerId);
    } else {
      const event = {
        mapId: row.mapId,
        date: row.timestamp,
        time: row.time,
        playerIds: [row.playerId],
      };
      byKey.set(key, event);
      events.push(event);
    }
  }

  return events;
}

export async function listRecentRecords() {
  const since = new Date(Date.now() - 7 * 24 * 3600 * 1000);
  const orderBy = [
    {
      timestamp: 'desc' as const,
    },
    {
      mapId: 'asc' as const,
    },
  ];

  let events = groupFeedEvents(
    await prisma.ddnetRecord.findMany({
      where: {
        timestamp: {
          gte: since,
        },
      },
      orderBy,
    })
  );

  if (events.length < 10) {
    events = groupFeedEvents(
      await prisma.ddnetRecord.findMany({
        orderBy,
        take: 200,
      })
    ).slice(0, 50);
  }

  const mapIds = [...new Set(events.map((event) => event.mapId))];

  const [maps, playerFor, history] = await Promise.all([
    prisma.map.findMany({
      select: {
        id: true,
        name: true,
        gameTypeName: true,
      },
      where: {
        id: {
          in: mapIds,
        },
      },
    }),
    playersById(events.flatMap((event) => event.playerIds)),
    prisma.ddnetRecord.findMany({
      select: {
        mapId: true,
        timestamp: true,
        time: true,
      },
      where: {
        mapId: {
          in: mapIds,
        },
      },
      orderBy: {
        timestamp: 'asc',
      },
    }),
  ]);

  const mapsById = new Map(maps.map((map) => [map.id, map]));

  const historyByMap = new Map<number, { timestamp: Date; time: number }[]>();

  for (const row of history) {
    const mapHistory = historyByMap.get(row.mapId) ?? [];
    mapHistory.push(row);
    historyByMap.set(row.mapId, mapHistory);
  }

  return events.flatMap((event) => {
    const map = mapsById.get(event.mapId);

    if (map === undefined) {
      return [];
    }

    let previousTime: number | null = null;

    for (const row of historyByMap.get(event.mapId) ?? []) {
      if (row.timestamp.getTime() >= event.date.getTime()) {
        break;
      }
      previousTime = row.time;
    }

    return [
      {
        map: {
          name: map.name,
          gameTypeName: map.gameTypeName,
        },
        players: event.playerIds
          .map(playerFor)
          .sort((a, b) => a.name.localeCompare(b.name)),
        time: event.time,
        delta: previousTime === null ? null : event.time - previousTime,
        date: event.date,
      },
    ];
  });
}

export async function getCompareSplits(mapId: number, playerName: string) {
  const player = await prisma.player.findUnique({
    select: {
      id: true,
    },
    where: {
      name: playerName,
    },
  });

  if (player === null) {
    return null;
  }

  const best = await prisma.ddnetRaceBest.findUnique({
    select: {
      splits: true,
    },
    where: {
      mapId_playerId: {
        mapId,
        playerId: player.id,
      },
    },
  });

  if (best === null || best.splits.length === 0) {
    return null;
  }

  return best.splits;
}

export async function getMapperMaps(mapperName: string) {
  const mapperRows = await prisma.ddnetMapMapper.findMany({
    where: {
      mapperName,
    },
  });

  if (mapperRows.length === 0) {
    return null;
  }

  const mapIds = mapperRows.map((row) => row.mapId);

  const [ddnetMaps, maps] = await Promise.all([
    prisma.ddnetMap.findMany({
      where: {
        mapId: {
          in: mapIds,
        },
      },
    }),
    prisma.map.findMany({
      select: {
        id: true,
        name: true,
        gameTypeName: true,
      },
      where: {
        id: {
          in: mapIds,
        },
      },
    }),
  ]);

  const mapsById = new Map(maps.map((map) => [map.id, map]));

  return ddnetMaps
    .flatMap((ddnetMap) => {
      const map = mapsById.get(ddnetMap.mapId);

      if (map === undefined) {
        return [];
      }

      return [
        {
          name: map.name,
          gameTypeName: map.gameTypeName,
          category: ddnetMap.category,
          releasedAt: ddnetMap.releasedAt,
          points: ddnetMap.points,
          finishCount: ddnetMap.finishCount,
          medianTime: ddnetMap.medianTime,
        },
      ];
    })
    .sort((a, b) => {
      if (a.releasedAt === null && b.releasedAt === null) {
        return a.name.localeCompare(b.name);
      }
      if (a.releasedAt === null) {
        return 1;
      }
      if (b.releasedAt === null) {
        return -1;
      }
      return (
        b.releasedAt.getTime() - a.releasedAt.getTime() ||
        a.name.localeCompare(b.name)
      );
    });
}
