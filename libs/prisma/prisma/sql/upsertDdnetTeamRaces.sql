-- $1: JSON-encoded array of {teamId (hex), mapId, time, timestamp, size}.
INSERT INTO "DdnetTeamRace" ("teamId", "mapId", "time", "timestamp", "size")
SELECT decode(t."teamId", 'hex'), t."mapId", t."time", t."timestamp", t."size"
FROM jsonb_to_recordset(($1::text)::jsonb) AS t(
  "teamId" text, "mapId" integer, "time" double precision,
  "timestamp" timestamp, "size" smallint
)
ON CONFLICT ("teamId") DO NOTHING;
