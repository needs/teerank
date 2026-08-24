SELECT "name"
FROM "Player"
WHERE "pollCount" < $1 OR "occurrenceCount"::float8 < "pollCount" * $2::float8
ORDER BY "playTime" DESC
LIMIT $3 OFFSET $4;
