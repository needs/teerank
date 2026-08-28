-- Full-table variant of refreshDdnetPoints: scans every best, so this runs
-- only at backfill tail and in the weekly full pass.
UPDATE "DdnetPlayer" p SET "points" = agg."points"
FROM (
  SELECT b."playerId", coalesce(sum(m."points"), 0)::int AS "points"
  FROM "DdnetRaceBest" b
  LEFT JOIN "DdnetMap" m ON m."mapId" = b."mapId"
  GROUP BY b."playerId"
) agg
WHERE p."playerId" = agg."playerId" AND p."points" IS DISTINCT FROM agg."points";
