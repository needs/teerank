SELECT count(*) AS "count"
FROM "PlayerPartner" AS pp
JOIN "Player" AS p ON p."id" = pp."partnerId"
WHERE pp."playerId" = $1
  AND (p."pollCount" < $2 OR p."occurrenceCount"::float8 < p."pollCount" * $3::float8);
