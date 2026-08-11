INSERT INTO "ClanInfoMap" ("clanName", "mapId", "playTime", "createdAt", "updatedAt")
SELECT t."clanName", $2::int4, t."playTime", now(), now()
FROM unnest($1::text[], $3::bigint[]) AS t("clanName", "playTime")
ON CONFLICT ("clanName", "mapId") DO UPDATE SET
  "playTime" = "ClanInfoMap"."playTime" + EXCLUDED."playTime",
  "updatedAt" = now();
