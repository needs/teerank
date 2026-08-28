SELECT count(*) AS "count"
FROM "DdnetRaceBest" AS rb
JOIN "Player" AS p ON p."id" = rb."playerId"
WHERE rb."mapId" = $1
  AND (p."pollCount" < $2 OR p."occurrenceCount"::float8 < p."pollCount" * $3::float8);
