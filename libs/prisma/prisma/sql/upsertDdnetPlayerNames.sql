-- Finishes prove presence, so lastSeenAt may move forward but never back;
-- historical backfill rows therefore never touch players seen by polling.
INSERT INTO "Player" ("name", "lastSeenAt", "createdAt", "updatedAt")
SELECT t."name", t."lastSeenAt", now(), now()
FROM unnest($1::text[], $2::timestamp[]) AS t("name", "lastSeenAt")
ON CONFLICT ("name") DO UPDATE SET
  "lastSeenAt" = EXCLUDED."lastSeenAt",
  "updatedAt" = now()
WHERE EXCLUDED."lastSeenAt" > "Player"."lastSeenAt";
