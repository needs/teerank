-- $1: JSON-encoded array of {teamId (hex), playerId}.
INSERT INTO "DdnetTeamRacePlayer" ("teamId", "playerId")
SELECT decode(t."teamId", 'hex'), t."playerId"
FROM jsonb_to_recordset(($1::text)::jsonb) AS t("teamId" text, "playerId" integer)
ON CONFLICT ("teamId", "playerId") DO NOTHING;
