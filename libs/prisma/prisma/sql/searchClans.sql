SELECT
  "public"."Clan"."name",
  "public"."Clan"."playTime",
  "public"."Clan"."activePlayerCount"
FROM
  "public"."Clan"
WHERE
  "public"."Clan"."name" ILIKE $1
ORDER BY
  LENGTH("public"."Clan"."name"),
  "public"."Clan"."playTime" DESC
LIMIT
  30;
