UPDATE "PlayerInfoMap" SET
  "rating" = t."rating",
  "updatedAt" = now()
FROM unnest($1::text[], $3::float8[]) AS t("playerName", "rating")
WHERE "PlayerInfoMap"."playerName" = t."playerName"
  AND "PlayerInfoMap"."mapId" = $2::int4;
