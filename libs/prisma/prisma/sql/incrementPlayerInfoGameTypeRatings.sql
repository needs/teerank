UPDATE "PlayerInfoGameType" SET
  "rating" = COALESCE("PlayerInfoGameType"."rating", 0) + t."eloDelta",
  "updatedAt" = now()
FROM unnest($1::int4[], $2::float8[]) AS t("id", "eloDelta")
WHERE "PlayerInfoGameType"."id" = t."id";
