-- DropIndex
DROP INDEX "GameServerSnapshot_createdAt_playTimingStartedAt_idx";

-- DropIndex
DROP INDEX "GameServerSnapshot_createdAt_rankingStartedAt_idx";

-- CreateIndex
CREATE INDEX "GameServerSnapshot_createdAt_rankingStartedAt_idx" ON "GameServerSnapshot"("createdAt" DESC, "rankingStartedAt" DESC);

-- CreateIndex
CREATE INDEX "GameServerSnapshot_createdAt_playTimingStartedAt_idx" ON "GameServerSnapshot"("createdAt" DESC, "playTimingStartedAt" DESC);
