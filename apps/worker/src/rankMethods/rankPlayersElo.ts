import { differenceInMinutes } from "date-fns";
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

export const rankPlayersElo = async (snapshotId: number) => {
  const { snapshot, snapshotBefore } = await getSnapshots(snapshotId);

  const playerInfoMaps = await Promise.all(
    snapshot.clients.map((client) => prisma.playerInfoMap.upsert({
      where: {
        playerName_mapId: {
          playerName: client.playerName,
          mapId: snapshot.mapId,
        },
      },
      select: {
        id: true,
        playerName: true,
        rating: true,
      },
      update: {},
      create: {
        playerName: client.playerName,
        mapId: snapshot.mapId,
        rating: 0,
      },
    })),
  );

  const mapRatings = new Map(playerInfoMaps.map(({ playerName, rating, id }) => [playerName, { rating: rating ?? 0, id }]));

  const playerInfoGameTypes = await Promise.all(
    snapshot.clients.map((client) => prisma.playerInfoGameType.upsert({
      where: {
        playerName_gameTypeName: {
          playerName: client.playerName,
          gameTypeName: snapshot.map.gameTypeName,
        },
      },
      select: {
        id: true,
        playerName: true,
        rating: true,
      },
      update: {},
      create: {
        playerName: client.playerName,
        gameTypeName: snapshot.map.gameTypeName,
        rating: 0,
      },
    })),
  );

  const gameTypeRatings = new Map(playerInfoGameTypes.map(({ playerName, rating, id }) => [playerName, { rating: rating ?? 0, id }]));

  if (snapshotBefore !== null) {
    const scoreDeltas = getScoreDeltas({ snapshotBefore, snapshot });

    const eloDeltasMap = computeEloDeltas(
      scoreDeltas,
      (playerName) => mapRatings.get(playerName)?.rating ?? 0,
    );

    if (eloDeltasMap.size > 0) {
      await Promise.all(Array.from(eloDeltasMap.entries()).filter(([, eloDelta]) => eloDelta !== 0).map(([playerName, eloDelta]) =>
        prisma.playerInfoMap.update({
          where: {
            id: mapRatings.get(playerName)?.id ?? 0,
          },
          data: {
            rating: {
              increment: eloDelta,
            },
          },
        })
      ));
    }

    const eloDeltasGameType = computeEloDeltas(
      scoreDeltas,
      (playerName) => gameTypeRatings.get(playerName)?.rating ?? 0,
    );

    if (eloDeltasGameType.size > 0) {
      await Promise.all(Array.from(eloDeltasGameType.entries()).filter(([, eloDelta]) => eloDelta !== 0).map(([playerName, eloDelta]) =>
        prisma.playerInfoGameType.update({
          where: {
            id: gameTypeRatings.get(playerName)?.id ?? 0,
          },
          data: {
            rating: {
              increment: eloDelta,
            },
          },
        })
      ));
    }
  }
}
