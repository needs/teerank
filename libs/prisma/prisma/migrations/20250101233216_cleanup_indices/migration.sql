-- DropIndex
DROP INDEX "GameServerClient_playerName_idx";

-- DropIndex
DROP INDEX "GameServerClient_score_inGame_idx";

-- DropIndex
DROP INDEX "GameServerStateClient_score_inGame_idx";

-- DropIndex
DROP INDEX "PlayerInfoMap_rating_idx";

-- DropIndex
DROP INDEX "PlayerInfoMap_rating_playTime_idx";

-- CreateIndex
CREATE INDEX "ClanInfoGameType_gameTypeName_playTime_idx" ON "ClanInfoGameType"("gameTypeName", "playTime" DESC);

-- CreateIndex
CREATE INDEX "ClanInfoMap_mapId_playTime_idx" ON "ClanInfoMap"("mapId", "playTime" DESC);

-- CreateIndex
CREATE INDEX "ClanPlayerInfo_playerName_idx" ON "ClanPlayerInfo"("playerName");

-- CreateIndex
CREATE INDEX "GameServer_createdAt_idx" ON "GameServer"("createdAt");

-- CreateIndex
CREATE INDEX "GameServerSnapshot_gameServerId_createdAt_idx" ON "GameServerSnapshot"("gameServerId", "createdAt");

-- CreateIndex
CREATE INDEX "Player_clanName_idx" ON "Player"("clanName");

-- CreateIndex
CREATE INDEX "PlayerInfoGameType_gameTypeName_rating_idx" ON "PlayerInfoGameType"("gameTypeName", "rating" DESC);

-- CreateIndex
CREATE INDEX "PlayerInfoMap_mapId_rating_idx" ON "PlayerInfoMap"("mapId", "rating" DESC);
