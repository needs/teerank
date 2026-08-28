-- Equal points share a rank; zero points means unranked (NULL).
UPDATE "DdnetPlayer" p SET "pointsRank" = r."rank"
FROM (
  SELECT "playerId",
    CASE WHEN "points" > 0 THEN (rank() OVER (ORDER BY "points" DESC))::int END AS "rank"
  FROM "DdnetPlayer"
) r
WHERE p."playerId" = r."playerId" AND p."pointsRank" IS DISTINCT FROM r."rank";
