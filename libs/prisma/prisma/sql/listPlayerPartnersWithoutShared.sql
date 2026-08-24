SELECT pp."partnerId", pp."playTime", pp."lastPlayedAt"
FROM "PlayerPartner" AS pp
JOIN "Player" AS p ON p."id" = pp."partnerId"
WHERE pp."playerId" = $1
  AND (p."pollCount" < $2 OR p."occurrenceCount"::float8 < p."pollCount" * $3::float8)
ORDER BY pp."playTime" DESC, pp."partnerId" ASC
LIMIT $4 OFFSET $5;
