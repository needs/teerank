SELECT
  "public"."Clan"."name",
  "public"."Clan"."playTime",
  COUNT(DISTINCT "public"."Player"."name") as "playerCount"
FROM
  "public"."Clan"
  LEFT JOIN "public"."Player" ON "public"."Player"."clanName" = "public"."Clan"."name"
WHERE
  "public"."Clan"."name" ILIKE $1
  AND "public"."Player"."clanName" IS NOT NULL
GROUP BY
  "public"."Clan"."name"
ORDER BY
  LENGTH("public"."Clan"."name"),
  "public"."Clan"."playTime" DESC
LIMIT
  30;
