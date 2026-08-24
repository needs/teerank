SELECT pim."playerName"
FROM "PlayerInfoMap" AS pim
JOIN "Map" AS m ON m."id" = pim."mapId"
JOIN "Player" AS p ON p."name" = pim."playerName"
WHERE m."name" = $1
  AND m."gameTypeName" = $2
  AND (p."pollCount" < $3 OR p."occurrenceCount"::float8 < p."pollCount" * $4::float8)
ORDER BY pim."rating" DESC NULLS LAST, pim."playTime" DESC
LIMIT $5 OFFSET $6;
