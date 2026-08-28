-- The latest record event per map is the current record.
SELECT DISTINCT ON ("mapId") "mapId", "time"
FROM "DdnetRecord"
ORDER BY "mapId", "timestamp" DESC;
