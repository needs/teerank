UPDATE "Player" AS p
SET
  "pollCount" = p."pollCount" + t."pollCount",
  "occurrenceCount" = p."occurrenceCount" + t."occurrenceCount"
FROM unnest($1::integer[], $2::integer[], $3::integer[]) AS t("playerId", "pollCount", "occurrenceCount")
WHERE p."id" = t."playerId";
