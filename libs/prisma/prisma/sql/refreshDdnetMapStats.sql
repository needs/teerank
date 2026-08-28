-- Recomputes per-map aggregates from the bests for the given maps: total
-- finishes, median/quantiles of best times, the record run's splits, and the
-- per-checkpoint median split (0 where no best has data for that checkpoint).
WITH affected AS (
  SELECT unnest($1::integer[]) AS "mapId"
),
best_stats AS (
  SELECT b."mapId",
    sum(b."finishCount")::int AS "finishCount",
    percentile_cont(0.5) WITHIN GROUP (ORDER BY b."time") AS "medianTime",
    (percentile_cont(ARRAY(SELECT g / 100.0 FROM generate_series(0, 100) g))
      WITHIN GROUP (ORDER BY b."time"))::real[] AS "timeQuantiles"
  FROM "DdnetRaceBest" b
  JOIN affected a ON a."mapId" = b."mapId"
  GROUP BY b."mapId"
),
wr AS (
  SELECT DISTINCT ON (b."mapId") b."mapId", b."splits"
  FROM "DdnetRaceBest" b
  JOIN affected a ON a."mapId" = b."mapId"
  ORDER BY b."mapId", b."time", b."playerId"
),
split_medians AS (
  SELECT s."mapId", array_agg(coalesce(s."median", 0)::real ORDER BY s."ord") AS "medianSplits"
  FROM (
    SELECT b."mapId", u."ord",
      percentile_cont(0.5) WITHIN GROUP (ORDER BY u."val") FILTER (WHERE u."val" > 0) AS "median"
    FROM "DdnetRaceBest" b
    JOIN affected a ON a."mapId" = b."mapId",
    LATERAL unnest(b."splits") WITH ORDINALITY AS u("val", "ord")
    GROUP BY b."mapId", u."ord"
  ) s
  GROUP BY s."mapId"
)
UPDATE "DdnetMap" m SET
  "finishCount" = bs."finishCount",
  "medianTime" = bs."medianTime",
  "timeQuantiles" = bs."timeQuantiles",
  "wrSplits" = coalesce(w."splits", '{}'),
  "medianSplits" = coalesce(sm."medianSplits", '{}')
FROM best_stats bs
LEFT JOIN wr w ON w."mapId" = bs."mapId"
LEFT JOIN split_medians sm ON sm."mapId" = bs."mapId"
WHERE m."mapId" = bs."mapId";
