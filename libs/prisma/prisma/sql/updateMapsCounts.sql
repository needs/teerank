UPDATE "Map" SET
  "playerCount" = counts."playerCount",
  "clanCount" = counts."clanCount",
  "gameServerCount" = counts."gameServerCount"
FROM (
  SELECT
    m.id,
    COALESCE(p.count, 0)::int4 AS "playerCount",
    COALESCE(c.count, 0)::int4 AS "clanCount",
    COALESCE(g.count, 0)::int4 AS "gameServerCount"
  FROM "Map" m
  LEFT JOIN (SELECT "mapId", count(*) AS count FROM "PlayerInfoMap" GROUP BY "mapId") p ON p."mapId" = m.id
  LEFT JOIN (SELECT "mapId", count(*) AS count FROM "ClanInfoMap" GROUP BY "mapId") c ON c."mapId" = m.id
  LEFT JOIN (SELECT "mapId", count(*) AS count FROM "GameServerState" GROUP BY "mapId") g ON g."mapId" = m.id
) counts
WHERE "Map".id = counts.id
  AND ("Map"."playerCount", "Map"."clanCount", "Map"."gameServerCount")
      IS DISTINCT FROM (counts."playerCount", counts."clanCount", counts."gameServerCount");
