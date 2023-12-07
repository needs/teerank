-- CreateIndex
CREATE INDEX "Clan_playTime_idx" ON "Clan"("playTime" DESC);

-- CreateIndex
CREATE INDEX "ClanInfo_playTime_idx" ON "ClanInfo"("playTime" DESC);

-- CreateIndex
CREATE INDEX "ClanPlayerInfo_playTime_idx" ON "ClanPlayerInfo"("playTime" DESC);

-- CreateIndex
CREATE INDEX "GameServerSnapshot_createdAt_idx" ON "GameServerSnapshot"("createdAt");

-- CreateIndex
CREATE INDEX "GameServerSnapshot_rankedAt_idx" ON "GameServerSnapshot"("rankedAt");

-- CreateIndex
CREATE INDEX "GameServerSnapshot_playTimedAt_idx" ON "GameServerSnapshot"("playTimedAt");

-- CreateIndex
CREATE INDEX "GameType_playTime_idx" ON "GameType"("playTime" DESC);

-- CreateIndex
CREATE INDEX "Map_playTime_idx" ON "Map"("playTime" DESC);

-- CreateIndex
CREATE INDEX "Player_playTime_idx" ON "Player"("playTime" DESC);

-- CreateIndex
CREATE INDEX "PlayerInfo_playTime_idx" ON "PlayerInfo"("playTime" DESC);
