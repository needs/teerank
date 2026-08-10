UPDATE "Clan" SET
  "playTime" = "Clan"."playTime" + t."playTime",
  "updatedAt" = now()
FROM unnest($1::text[], $2::bigint[]) AS t("name", "playTime")
WHERE "Clan"."name" = t."name";
