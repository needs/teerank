-- DropIndex
DROP INDEX "GameServerSnapshot_gameServerPlayTimingStartedAt_idx";

-- CreateIndex
CREATE INDEX "GameServerSnapshot_gameServerPlayTimingStartedAt_gameServer_idx" ON "GameServerSnapshot"("gameServerPlayTimingStartedAt", "gameServerPlayTimedAt");
