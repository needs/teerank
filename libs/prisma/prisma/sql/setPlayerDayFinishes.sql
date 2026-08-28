-- Authoritative recount for recent days: overwrites whatever is there, which
-- also restores counts wiped by a re-rollup of the same day.
INSERT INTO "PlayerDay" ("day", "playerId", "playTime", "finishCount")
SELECT t."day", t."playerId", 0, t."finishCount"
FROM unnest($1::date[], $2::integer[], $3::integer[]) AS t("day", "playerId", "finishCount")
ON CONFLICT ("day", "playerId") DO UPDATE SET
  "finishCount" = EXCLUDED."finishCount";
