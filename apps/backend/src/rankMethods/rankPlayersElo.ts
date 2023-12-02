import { differenceInMinutes } from "date-fns";
import { RankFunctionArgs, SnapshotToRank } from "../tasks/rankPlayers";

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
  snapshots,
  getMapRating,
  setMapRating,
  getGameTypeRating,
  setGameTypeRating,
  markAsRanked,
}: RankFunctionArgs) => {
  for (let index = 0; index < snapshots.length - 1; index++) {
    const snapshotStart = snapshots[index];
    const snapshotEnd = snapshots[index + 1];

    const scoreDeltas = getScoreDeltas(snapshotStart, snapshotEnd);

    if (scoreDeltas !== undefined) {
      const newMapRatings: TemporaryRatings = [];

      updateRatings(
        scoreDeltas,
        (playerName) => getMapRating(snapshotStart.mapId, playerName),
        (playerName, rating) => newMapRatings.push({ playerName, rating })
      );

      for (const { playerName, rating } of newMapRatings) {
        setMapRating(snapshotStart.mapId, playerName, rating);
      }

      const newGameTypeRatings: TemporaryRatings = [];

      updateRatings(
        scoreDeltas,
        (playerName) => getGameTypeRating(snapshotStart.map.gameTypeName, playerName),
        (playerName, rating) => newGameTypeRatings.push({ playerName, rating })
      );

      for (const { playerName, rating } of newGameTypeRatings) {
        setGameTypeRating(snapshotStart.map.gameTypeName, playerName, rating);
      }
    }

    markAsRanked(snapshotStart.id);
  }
}
