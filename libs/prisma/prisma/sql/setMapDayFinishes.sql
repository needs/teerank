INSERT INTO "MapDay" ("day", "mapId", "playTime", "playerCount", "finishCount")
SELECT t."day", t."mapId", 0, 0, t."finishCount"
FROM unnest($1::date[], $2::integer[], $3::integer[]) AS t("day", "mapId", "finishCount")
ON CONFLICT ("mapId", "day") DO UPDATE SET
  "finishCount" = EXCLUDED."finishCount";
