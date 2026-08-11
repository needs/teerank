INSERT INTO "ClanInfoGameType" ("clanName", "gameTypeName", "playTime", "createdAt", "updatedAt")
SELECT t."clanName", $2, t."playTime", now(), now()
FROM unnest($1::text[], $3::bigint[]) AS t("clanName", "playTime")
ON CONFLICT ("clanName", "gameTypeName") DO UPDATE SET
  "playTime" = "ClanInfoGameType"."playTime" + EXCLUDED."playTime",
  "updatedAt" = now();
