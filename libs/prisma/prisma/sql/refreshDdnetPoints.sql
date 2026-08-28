-- Points = sum of map points over distinct finished maps. LEFT JOIN so a
-- player whose only pointed map was removed drops back to 0.
UPDATE "DdnetPlayer" p SET "points" = agg."points"
FROM (
  SELECT b."playerId", coalesce(sum(m."points"), 0)::int AS "points"
  FROM "DdnetRaceBest" b
  LEFT JOIN "DdnetMap" m ON m."mapId" = b."mapId"
  WHERE b."playerId" = ANY($1::integer[])
  GROUP BY b."playerId"
) agg
WHERE p."playerId" = agg."playerId" AND p."points" IS DISTINCT FROM agg."points";
