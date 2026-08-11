UPDATE "Player" SET
  "playTime" = "Player"."playTime" + t."playTime",
  "updatedAt" = now()
FROM unnest($1::text[], $2::bigint[]) AS t("name", "playTime")
WHERE "Player"."name" = t."name";
