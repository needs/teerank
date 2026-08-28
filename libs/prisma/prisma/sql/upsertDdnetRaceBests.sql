-- $1: JSON-encoded array of {mapId, playerId, time, finishCount,
-- firstFinishAt, bestAt, splits} where splits is a (possibly empty) array of
-- seconds. LEAST/CASE merges keep the statement idempotent for the min fields.
INSERT INTO "DdnetRaceBest" ("mapId", "playerId", "time", "finishCount", "firstFinishAt", "bestAt", "splits")
SELECT t."mapId", t."playerId", t."time", t."finishCount", t."firstFinishAt", t."bestAt",
  coalesce(ARRAY(SELECT value::real FROM jsonb_array_elements_text(t."splits")), '{}')
FROM jsonb_to_recordset(($1::text)::jsonb) AS t(
  "mapId" integer, "playerId" integer, "time" double precision,
  "finishCount" integer, "firstFinishAt" date, "bestAt" date, "splits" jsonb
)
ON CONFLICT ("mapId", "playerId") DO UPDATE SET
  "finishCount" = "DdnetRaceBest"."finishCount" + EXCLUDED."finishCount",
  "firstFinishAt" = LEAST("DdnetRaceBest"."firstFinishAt", EXCLUDED."firstFinishAt"),
  "bestAt" = CASE WHEN EXCLUDED."time" < "DdnetRaceBest"."time"
    THEN EXCLUDED."bestAt" ELSE "DdnetRaceBest"."bestAt" END,
  "splits" = CASE WHEN EXCLUDED."time" < "DdnetRaceBest"."time"
    THEN EXCLUDED."splits" ELSE "DdnetRaceBest"."splits" END,
  "time" = LEAST("DdnetRaceBest"."time", EXCLUDED."time");
