SELECT rb."time", rb."bestAt", p."name", p."pollCount", p."occurrenceCount"
FROM "DdnetRaceBest" AS rb
JOIN "Player" AS p ON p."id" = rb."playerId"
WHERE rb."mapId" = $1
  AND (p."pollCount" < $2 OR p."occurrenceCount"::float8 < p."pollCount" * $3::float8)
ORDER BY rb."time" ASC, rb."playerId" ASC
LIMIT $4 OFFSET $5;
