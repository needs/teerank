INSERT INTO "ClanPlayerInfo" ("clanName", "playerName", "playTime", "createdAt", "updatedAt")
SELECT t."clanName", t."playerName", t."playTime", now(), now()
FROM unnest($1::text[], $2::text[], $3::bigint[]) AS t("clanName", "playerName", "playTime")
ON CONFLICT ("clanName", "playerName") DO UPDATE SET
  "playTime" = "ClanPlayerInfo"."playTime" + EXCLUDED."playTime",
  "updatedAt" = now();
