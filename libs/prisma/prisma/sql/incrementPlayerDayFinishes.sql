INSERT INTO "PlayerDay" ("day", "playerId", "playTime", "finishCount")
SELECT t."day", t."playerId", 0, t."finishCount"
FROM unnest($1::date[], $2::integer[], $3::integer[]) AS t("day", "playerId", "finishCount")
ON CONFLICT ("day", "playerId") DO UPDATE SET
  "finishCount" = "PlayerDay"."finishCount" + EXCLUDED."finishCount";
