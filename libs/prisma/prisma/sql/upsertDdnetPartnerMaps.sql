INSERT INTO "DdnetPartnerMap" ("playerId", "partnerId", "mapId", "bestTime", "finishCount", "lastFinishAt")
SELECT t."playerId", t."partnerId", t."mapId", t."bestTime", t."finishCount", t."lastFinishAt"
FROM unnest(
  $1::integer[], $2::integer[], $3::integer[],
  $4::double precision[], $5::integer[], $6::date[]
) AS t("playerId", "partnerId", "mapId", "bestTime", "finishCount", "lastFinishAt")
ON CONFLICT ("playerId", "partnerId", "mapId") DO UPDATE SET
  "finishCount" = "DdnetPartnerMap"."finishCount" + EXCLUDED."finishCount",
  "bestTime" = LEAST("DdnetPartnerMap"."bestTime", EXCLUDED."bestTime"),
  "lastFinishAt" = GREATEST("DdnetPartnerMap"."lastFinishAt", EXCLUDED."lastFinishAt");
