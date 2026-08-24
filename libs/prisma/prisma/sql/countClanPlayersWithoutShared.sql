SELECT count(*) AS "count"
FROM "ClanPlayerInfo" AS cpi
JOIN "Player" AS p ON p."name" = cpi."playerName"
WHERE cpi."clanName" = $1
  AND ($2::bool OR p."clanName" = $1)
  AND (p."pollCount" < $3 OR p."occurrenceCount"::float8 < p."pollCount" * $4::float8);
