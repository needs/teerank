UPDATE "Map" SET
  "gameServerCount" = counts."gameServerCount"
FROM (
  SELECT
    m.id,
    COALESCE(g.count, 0)::int4 AS "gameServerCount"
  FROM "Map" m
  LEFT JOIN (SELECT "mapId", count(*) AS count FROM "GameServerState" GROUP BY "mapId") g ON g."mapId" = m.id
) counts
WHERE "Map".id = counts.id
  AND "Map"."gameServerCount" IS DISTINCT FROM counts."gameServerCount";
