-- DropIndex
DROP INDEX "GameServerSnapshot_gameServerPlayTimingStartedAt_gameServer_idx";

-- CreateIndex
CREATE INDEX "GameServerSnapshot_gameServerPlayTimedAt_idx" ON "GameServerSnapshot"("gameServerPlayTimedAt");

-- CreateIndex
CREATE INDEX "GameServerSnapshot_gameServerPlayTimingStartedAt_idx" ON "GameServerSnapshot"("gameServerPlayTimingStartedAt");
