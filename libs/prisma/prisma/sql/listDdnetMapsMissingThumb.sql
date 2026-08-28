SELECT m."mapId", map."name"
FROM "DdnetMap" AS m
JOIN "Map" AS map ON map."id" = m."mapId"
LEFT JOIN "DdnetMapThumb" AS t ON t."mapId" = m."mapId"
WHERE t."mapId" IS NULL
ORDER BY m."mapId"
LIMIT $1;
