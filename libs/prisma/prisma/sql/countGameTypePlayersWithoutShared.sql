SELECT count(*) AS "count"
FROM "PlayerInfoGameType" AS pig
JOIN "Player" AS p ON p."name" = pig."playerName"
WHERE pig."gameTypeName" = $1
  AND (p."pollCount" < $2 OR p."occurrenceCount"::float8 < p."pollCount" * $3::float8);
