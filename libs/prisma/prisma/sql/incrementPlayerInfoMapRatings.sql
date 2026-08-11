UPDATE "PlayerInfoMap" SET
  "rating" = COALESCE("PlayerInfoMap"."rating", 0) + t."eloDelta",
  "updatedAt" = now()
FROM unnest($1::int4[], $2::float8[]) AS t("id", "eloDelta")
WHERE "PlayerInfoMap"."id" = t."id";
