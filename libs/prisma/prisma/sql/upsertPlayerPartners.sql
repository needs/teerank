INSERT INTO "PlayerPartner" ("playerId", "partnerId", "playTime", "lastPlayedAt")
SELECT t."playerId", t."partnerId", t."playTime", $4::date
FROM unnest($1::integer[], $2::integer[], $3::integer[]) AS t("playerId", "partnerId", "playTime")
ON CONFLICT ("playerId", "partnerId") DO UPDATE SET
  "playTime" = "PlayerPartner"."playTime" + EXCLUDED."playTime",
  "lastPlayedAt" = GREATEST("PlayerPartner"."lastPlayedAt", EXCLUDED."lastPlayedAt");
