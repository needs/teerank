INSERT INTO "PlayerInfoGameType" ("playerName", "gameTypeName", "playTime", "createdAt", "updatedAt")
SELECT t."playerName", $2, t."playTime", now(), now()
FROM unnest($1::text[], $3::bigint[]) AS t("playerName", "playTime")
ON CONFLICT ("playerName", "gameTypeName") DO UPDATE SET
  "playTime" = "PlayerInfoGameType"."playTime" + EXCLUDED."playTime",
  "updatedAt" = now();
