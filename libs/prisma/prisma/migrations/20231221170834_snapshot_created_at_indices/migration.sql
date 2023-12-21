-- CreateIndex
CREATE INDEX "GameServerSnapshot_createdAt_rankedAt_idx" ON "GameServerSnapshot"("createdAt" DESC, "rankedAt");

-- CreateIndex
CREATE INDEX "GameServerSnapshot_createdAt_playTimedAt_idx" ON "GameServerSnapshot"("createdAt" DESC, "playTimedAt");
