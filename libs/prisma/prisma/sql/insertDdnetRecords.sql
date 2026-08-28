INSERT INTO "DdnetRecord" ("mapId", "timestamp", "playerId", "time")
SELECT t."mapId", t."timestamp", t."playerId", t."time"
FROM unnest($1::integer[], $2::timestamp[], $3::integer[], $4::double precision[])
  AS t("mapId", "timestamp", "playerId", "time")
ON CONFLICT ("mapId", "timestamp", "playerId") DO NOTHING;
