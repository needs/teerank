INSERT INTO "PlayerInfoMap" ("playerName", "mapId", "playTime", "createdAt", "updatedAt")
SELECT t."playerName", $2::int4, t."playTime", now(), now()
FROM unnest($1::text[], $3::bigint[]) AS t("playerName", "playTime")
ON CONFLICT ("playerName", "mapId") DO UPDATE SET
  "playTime" = "PlayerInfoMap"."playTime" + EXCLUDED."playTime",
  "updatedAt" = now();
