import { differenceInMinutes } from "date-fns";
import { incrementPlayerInfoGameTypeRatings, incrementPlayerInfoMapRatings } from "@prisma/client/sql";
import { prisma } from "../prisma";

async function getSnapshots(snapshotId: number) {
  const snapshot = await prisma.gameServerSnapshot.findUniqueOrThrow({
    where: {
      id: snapshotId,
    },
    select: {
      id: true,
      createdAt: true,
      gameServerId: true,
      clients: {
        select: {
          playerName: true,
          score: true,
          inGame: true,
        }
      },
      mapId: true,
      map: {
        select: {
          gameTypeName: true,
        },
      },
    },
  });

  const snapshotBefore = await prisma.gameServerSnapshot.findFirst({
    select: {
      id: true,
      createdAt: true,
      gameServerId: true,
      clients: {
        select: {
          playerName: true,
          score: true,
          inGame: true,
        }
      },
      mapId: true,
      map: {
        select: {
          gameTypeName: true,
        },
      },
    },
    where: {
      createdAt: {
        lt: snapshot.createdAt,
      },
      gameServerId: snapshot.gameServerId,
    },
    orderBy: {
      createdAt: 'desc',
    },
  });

  return { snapshot, snapshotBefore };
}

type ScoreDeltas = Map<string, number | undefined>;

function getScoreDeltas({ snapshotBefore, snapshot }: Awaited<ReturnType<typeof getSnapshots>>): ScoreDeltas {
  if (snapshotBefore === null) {
    return new Map();
  }

  // Ranking on a different map or gametype doesn't make sense.
  if (snapshotBefore.mapId !== snapshot.mapId) {
    return new Map();
  }

  // More than 30 minutes between snapshots increases odds to rank a different game.
  if (differenceInMinutes(snapshot.createdAt, snapshotBefore.createdAt) > 30) {
    return new Map();
  }

  const scoreDeltas = new Map<string, number>();

  for (const clientStart of snapshotBefore.clients) {
    const clientEnd = snapshot.clients.find((clientEnd) =>
      clientEnd.playerName === clientStart.playerName
    )

    if (clientEnd !== undefined && clientStart.inGame && clientEnd.inGame) {
      scoreDeltas.set(clientStart.playerName, clientEnd.score - clientStart.score);
    }
  }

  // At least two players are needed to rank.
  if (scoreDeltas.size < 2) {
    return new Map();
  }

  // If average score is less than -1, then it's probably a new game.
  const scoreAverage = [...scoreDeltas.values()].reduce((sum, scoreDelta) => sum + scoreDelta, 0) / scoreDeltas.size;
  if (scoreAverage < -1) {
    return new Map();
  }

  return scoreDeltas;
}

// Classic Elo formula for two players
function computeEloDelta(scoreA: number, eloA: number, scoreB: number, eloB: number) {
  const K = 25;

  // p() func as defined by Elo.
  function p(delta: number) {
    const clampedDelta = Math.max(-400, Math.min(400, delta));
    return 1.0 / (1.0 + Math.pow(10.0, -clampedDelta / 400.0));
  }

  let W = 0.5;

  if (scoreA < scoreB) {
    W = 0;
  } else if (scoreA > scoreB) {
    W = 1;
  }

  return K * (W - p(eloA - eloB));
}

function computeEloDeltas(
  scoreDeltas: ScoreDeltas,
  getRating: (playerName: string) => number,
) {
  const eloDeltas = new Map<string, number>();

  for (const [playerName, scoreDelta] of scoreDeltas.entries()) {
    let eloSum = 0;

    for (const [otherPlayerName, otherScoreDelta] of scoreDeltas.entries()) {
      if (playerName !== otherPlayerName && scoreDelta !== undefined && otherScoreDelta !== undefined) {
        eloSum += computeEloDelta(
          scoreDelta,
          getRating(playerName),
          otherScoreDelta,
          getRating(otherPlayerName)
        );
      }
    }

    const eloDelta = eloSum / (scoreDeltas.size - 1);
    eloDeltas.set(playerName, eloDelta);
  }

  return eloDeltas;
}

function compareStrings(a: string, b: string) {
  return a < b ? -1 : a > b ? 1 : 0;
}

// Fetch the info rows for the snapshot's players, creating the (rarely)
// missing ones with a 0 rating first. Two statements instead of one upsert
// per player — a plain read in steady state, since rows exist after a
// player's first ranked snapshot.
async function getPlayerInfoMaps(playerNames: string[], mapId: number) {
  const where = {
    mapId,
    playerName: {
      in: playerNames,
    },
  };
  const select = {
    id: true,
    playerName: true,
    rating: true,
  };

  let playerInfoMaps = await prisma.playerInfoMap.findMany({ where, select });

  if (playerInfoMaps.length < playerNames.length) {
    const existing = new Set(playerInfoMaps.map(({ playerName }) => playerName));
    const missing = playerNames.filter((playerName) => !existing.has(playerName));

    await prisma.playerInfoMap.createMany({
      data: missing.map((playerName) => ({ playerName, mapId, rating: 0 })),
      skipDuplicates: true,
    });
    playerInfoMaps = await prisma.playerInfoMap.findMany({ where, select });
  }

  return playerInfoMaps;
}

async function getPlayerInfoGameTypes(playerNames: string[], gameTypeName: string) {
  const where = {
    gameTypeName,
    playerName: {
      in: playerNames,
    },
  };
  const select = {
    id: true,
    playerName: true,
    rating: true,
  };

  let playerInfoGameTypes = await prisma.playerInfoGameType.findMany({ where, select });

  if (playerInfoGameTypes.length < playerNames.length) {
    const existing = new Set(playerInfoGameTypes.map(({ playerName }) => playerName));
    const missing = playerNames.filter((playerName) => !existing.has(playerName));

    await prisma.playerInfoGameType.createMany({
      data: missing.map((playerName) => ({ playerName, gameTypeName, rating: 0 })),
      skipDuplicates: true,
    });
    playerInfoGameTypes = await prisma.playerInfoGameType.findMany({ where, select });
  }

  return playerInfoGameTypes;
}

async function incrementRatings(table: 'PlayerInfoMap' | 'PlayerInfoGameType', eloDeltas: Map<string, number>, ids: Map<string, { id: number }>) {
  // Sorted by id, the conflict key, to avoid deadlocks between concurrent
  // multi-row updates.
  const updates = [...eloDeltas.entries()]
    .filter(([, eloDelta]) => eloDelta !== 0)
    .map(([playerName, eloDelta]) => ({ id: ids.get(playerName)?.id ?? 0, eloDelta }))
    .sort((a, b) => a.id - b.id);

  if (updates.length === 0) {
    return;
  }

  const updateIds = updates.map((update) => update.id);
  const updateDeltas = updates.map((update) => update.eloDelta);

  if (table === 'PlayerInfoMap') {
    await prisma.$queryRawTyped(incrementPlayerInfoMapRatings(updateIds, updateDeltas));
  } else {
    await prisma.$queryRawTyped(incrementPlayerInfoGameTypeRatings(updateIds, updateDeltas));
  }
}

export const rankPlayersElo = async (snapshotId: number) => {
  const { snapshot, snapshotBefore } = await getSnapshots(snapshotId);

  if (snapshotBefore === null) {
    return;
  }

  const scoreDeltas = getScoreDeltas({ snapshotBefore, snapshot });

  if (scoreDeltas.size === 0) {
    return;
  }

  const playerNames = [...new Set(snapshot.clients.map((client) => client.playerName))].sort();

  const playerInfoMaps = await getPlayerInfoMaps(playerNames, snapshot.mapId);
  const mapRatings = new Map(playerInfoMaps.map(({ playerName, rating, id }) => [playerName, { rating: rating ?? 0, id }]));

  const playerInfoGameTypes = await getPlayerInfoGameTypes(playerNames, snapshot.map.gameTypeName);
  const gameTypeRatings = new Map(playerInfoGameTypes.map(({ playerName, rating, id }) => [playerName, { rating: rating ?? 0, id }]));

  const eloDeltasMap = computeEloDeltas(
    scoreDeltas,
    (playerName) => mapRatings.get(playerName)?.rating ?? 0,
  );

  await incrementRatings('PlayerInfoMap', eloDeltasMap, mapRatings);

  const eloDeltasGameType = computeEloDeltas(
    scoreDeltas,
    (playerName) => gameTypeRatings.get(playerName)?.rating ?? 0,
  );

  await incrementRatings('PlayerInfoGameType', eloDeltasGameType, gameTypeRatings);
}
