-- An empty clan means "no clan info": it inserts as NULL and never
-- overwrites a stored clan. The DO UPDATE is a no-op unless lastSeenAt
-- moved by more than 10 minutes or the clan changed.
INSERT INTO "Player" ("name", "clanName", "lastSeenAt", "createdAt", "updatedAt")
SELECT t."name", NULLIF(t."clanName", ''), now(), now(), now()
FROM unnest($1::text[], $2::text[]) AS t("name", "clanName")
ON CONFLICT ("name") DO UPDATE SET
  "clanName" = COALESCE(EXCLUDED."clanName", "Player"."clanName"),
  "lastSeenAt" = EXCLUDED."lastSeenAt",
  "updatedAt" = EXCLUDED."updatedAt"
WHERE EXCLUDED."lastSeenAt" - "Player"."lastSeenAt" > interval '10 minutes'
  OR (EXCLUDED."clanName" IS NOT NULL AND EXCLUDED."clanName" IS DISTINCT FROM "Player"."clanName");
