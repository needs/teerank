-- Recomputes points for every finisher of the given maps (used when a map's
-- points are re-rated or a map is added/removed).
UPDATE "DdnetPlayer" p SET "points" = agg."points"
FROM (
  SELECT b."playerId", coalesce(sum(m."points"), 0)::int AS "points"
  FROM "DdnetRaceBest" b
  LEFT JOIN "DdnetMap" m ON m."mapId" = b."mapId"
  WHERE b."playerId" IN (
    SELECT DISTINCT f."playerId" FROM "DdnetRaceBest" f WHERE f."mapId" = ANY($1::integer[])
  )
  GROUP BY b."playerId"
) agg
WHERE p."playerId" = agg."playerId" AND p."points" IS DISTINCT FROM agg."points";
