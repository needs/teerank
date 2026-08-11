-- Orphaned states can no longer be created: pollGameServer now upserts the state
-- in place instead of disconnect-and-recreate.
DELETE FROM "GameServerState" WHERE "gameServerId" IS NULL;

-- AlterColumn
ALTER TABLE "GameServerState" ALTER COLUMN "gameServerId" SET NOT NULL;
