SELECT
  "public"."Player"."name",
  "public"."Player"."lastSeenAt",
  "public"."Player"."clanName",
  "public"."Player"."playTime",
  array_agg(
    DISTINCT jsonb_build_object(
      'ip', "public"."GameServer"."ip",
      'port', "public"."GameServer"."port"
    )
  ) FILTER (WHERE "public"."GameServer"."ip" IS NOT NULL)
  as "servers"
FROM
  "public"."Player"
  LEFT JOIN "public"."GameServerStateClient" ON "public"."GameServerStateClient"."playerName" = "public"."Player"."name"
  LEFT JOIN "public"."GameServerState" ON "public"."GameServerState"."id" = "public"."GameServerStateClient"."gameServerStateId"
  LEFT JOIN "public"."GameServer" ON "public"."GameServer"."id" = "public"."GameServerState"."gameServerId"
WHERE
  "public"."Player"."name" ILIKE $1
GROUP BY
  "public"."Player"."name",
  "public"."Player"."lastSeenAt",
  "public"."Player"."clanName",
  "public"."Player"."playTime"
ORDER BY
  LENGTH("public"."Player"."name"),
  "public"."Player"."playTime" DESC
LIMIT
  30;
