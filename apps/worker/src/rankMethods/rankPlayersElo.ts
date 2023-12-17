import { differenceInMinutes } from "date-fns";
import { RankFunctionArgs, SnapshotToRank } from "../tasks/rankPlayers";
import { prisma } from "../prisma";

type ScoreDeltas = Map<string, number>;

function getScoreDeltas(snapshotStart: SnapshotToRank, snapshotEnd: SnapshotToRank): ScoreDeltas | undefined {
  // Ranking on a different map or gametype doesn't make sense.
  if (snapshotStart.mapId !== snapshotEnd.mapId) {
    return undefined;
  }

  // More than 30 minutes between snapshots increases odds to rank a different game.
  if (differenceInMinutes(snapshotEnd.createdAt, snapshotStart.createdAt) > 30) {
    return undefined;
  }

  // Get common players from both snapshots
  const scoreDeltas = new Map<string, number>();

  for (const clientStart of snapshotStart.clients) {
    const clientEnd = snapshotEnd.clients.find((clientEnd) =>
      clientEnd.playerName === clientStart.playerName
    )

    if (clientEnd !== undefined && clientStart.inGame && clientEnd.inGame) {
      scoreDeltas.set(clientStart.playerName, clientEnd.score - clientStart.score);
    }
  }

  // At least two players are needed to rank.
  if (scoreDeltas.size < 2) {
    return undefined;
  }

  // If average score is less than -1, then it's probably a new game.
  const scoreAverage = [...scoreDeltas.values()].reduce((sum, scoreDelta) => sum + scoreDelta, 0) / scoreDeltas.size;
  if (scoreAverage < -1) {
    return undefined;
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

function updateRatings(
  scoreDeltas: ScoreDeltas,
  getRating: (playerName: string) => number | undefined,
  setRating: (playerName: string, rating: number) => void
) {
  for (const [playerName, scoreDelta] of scoreDeltas.entries()) {
    let eloSum = 0;

    for (const [otherPlayerName, otherScoreDelta] of scoreDeltas.entries()) {
      if (playerName !== otherPlayerName) {
        eloSum += computeEloDelta(
          scoreDelta,
          getRating(playerName) ?? 0,
          otherScoreDelta,
          getRating(otherPlayerName) ?? 0
        );
      }
    }

    const eloAverage = eloSum / (scoreDeltas.size - 1);
    setRating(playerName, (getRating(playerName) ?? 0) + eloAverage);
  }
}

type TemporaryRatings = { playerName: string, rating: number }[];

export const rankPlayersElo = async ({
  snapshot,
  getMapRating,
  setMapRating,
  getGameTypeRating,
  setGameTypeRating,
}: RankFunctionArgs) => {
  const snapshotBefore = await prisma.gameServerSnapshot.findFirst({
    select: {
      id: true,
      createdAt: true,
      gameServerId: true,
      clients: {
        where: {
          playerName: {
            // Don't rank connecting players because their score is meaningless.
            not: "(connecting)",
          }
        },
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
          gameType: {
            select: {
              rankMethod: true,
            }
          },
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
  }) ?? snapshot;

  for (const client of snapshot.clients) {
    const rating = getMapRating(snapshot.mapId, client.playerName) ?? 0;
    setMapRating(snapshot.mapId, client.playerName, rating);

    const ratingGameType = getGameTypeRating(snapshot.map.gameTypeName, client.playerName) ?? 0;
    setGameTypeRating(snapshot.map.gameTypeName, client.playerName, ratingGameType);
  }

  const scoreDeltas = getScoreDeltas(snapshotBefore, snapshot);
  if (scoreDeltas !== undefined) {
    const newMapRatings: TemporaryRatings = [];

    updateRatings(
      scoreDeltas,
      (playerName) => getMapRating(snapshot.mapId, playerName),
      (playerName, rating) => newMapRatings.push({ playerName, rating })
    );

    for (const { playerName, rating } of newMapRatings) {
      setMapRating(snapshot.mapId, playerName, rating);
    }

    const newGameTypeRatings: TemporaryRatings = [];

    updateRatings(
      scoreDeltas,
      (playerName) => getGameTypeRating(snapshot.map.gameTypeName, playerName),
      (playerName, rating) => newGameTypeRatings.push({ playerName, rating })
    );

    for (const { playerName, rating } of newGameTypeRatings) {
      setGameTypeRating(snapshot.map.gameTypeName, playerName, rating);
    }
  }
}
