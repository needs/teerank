SELECT count(*) AS "count"
FROM "Player"
WHERE "pollCount" < $1 OR "occurrenceCount"::float8 < "pollCount" * $2::float8;
