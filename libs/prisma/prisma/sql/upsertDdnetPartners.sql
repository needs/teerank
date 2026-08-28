INSERT INTO "DdnetPartner" ("playerId", "partnerId", "finishCount", "bestTime", "bestMapId", "lastFinishAt")
SELECT t."playerId", t."partnerId", t."finishCount", t."bestTime", t."bestMapId", t."lastFinishAt"
FROM unnest(
  $1::integer[], $2::integer[], $3::integer[],
  $4::double precision[], $5::integer[], $6::date[]
) AS t("playerId", "partnerId", "finishCount", "bestTime", "bestMapId", "lastFinishAt")
ON CONFLICT ("playerId", "partnerId") DO UPDATE SET
  "finishCount" = "DdnetPartner"."finishCount" + EXCLUDED."finishCount",
  "bestMapId" = CASE WHEN EXCLUDED."bestTime" < "DdnetPartner"."bestTime"
    THEN EXCLUDED."bestMapId" ELSE "DdnetPartner"."bestMapId" END,
  "bestTime" = LEAST("DdnetPartner"."bestTime", EXCLUDED."bestTime"),
  "lastFinishAt" = GREATEST("DdnetPartner"."lastFinishAt", EXCLUDED."lastFinishAt");
