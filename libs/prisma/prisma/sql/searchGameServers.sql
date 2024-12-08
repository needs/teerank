SELECT
  "public"."GameServer"."ip",
  "public"."GameServer"."port",
  "public"."GameServerState"."name",
  "public"."GameServerState"."numClients",
  "public"."GameServerState"."maxClients",
  "public"."Map"."name" as "mapName",
  "public"."Map"."gameTypeName"
FROM
  "public"."GameServerState"
  INNER JOIN "public"."GameServer" ON "public"."GameServer"."id" = "public"."GameServerState"."gameServerId"
  LEFT JOIN "public"."Map" ON "public"."GameServerState"."mapId" = "public"."Map"."id"
WHERE
  "public"."GameServerState"."name" ILIKE $1
ORDER BY
  LENGTH("public"."GameServerState"."name"),
  "public"."GameServerState"."numClients" DESC
LIMIT
  30;
