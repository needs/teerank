SELECT pig."playerName"
FROM "PlayerInfoGameType" AS pig
JOIN "Player" AS p ON p."name" = pig."playerName"
WHERE pig."gameTypeName" = $1
  AND (p."pollCount" < $2 OR p."occurrenceCount"::float8 < p."pollCount" * $3::float8)
ORDER BY pig."rating" DESC NULLS LAST, pig."playTime" DESC
LIMIT $4 OFFSET $5;
