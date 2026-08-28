INSERT INTO "DdnetPlayer" ("playerId", "finishCount", "firstFinishAt", "lastFinishAt")
SELECT t."playerId", t."finishCount", t."firstFinishAt", t."lastFinishAt"
FROM unnest($1::integer[], $2::integer[], $3::timestamp[], $4::timestamp[])
  AS t("playerId", "finishCount", "firstFinishAt", "lastFinishAt")
ON CONFLICT ("playerId") DO UPDATE SET
  "finishCount" = "DdnetPlayer"."finishCount" + EXCLUDED."finishCount",
  "firstFinishAt" = LEAST("DdnetPlayer"."firstFinishAt", EXCLUDED."firstFinishAt"),
  "lastFinishAt" = GREATEST("DdnetPlayer"."lastFinishAt", EXCLUDED."lastFinishAt");
