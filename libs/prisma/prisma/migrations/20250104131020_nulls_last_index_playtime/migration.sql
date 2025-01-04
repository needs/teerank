-- DropIndex
DROP INDEX "PlayerInfoGameType_gameTypeName_rating_idx";

-- DropIndex
DROP INDEX "PlayerInfoMap_mapId_rating_idx";

-- CreateIndex
CREATE INDEX "PlayerInfoGameType_gameTypeName_rating_playTime_idx" ON "PlayerInfoGameType"("gameTypeName", "rating" DESC NULLS LAST, "playTime" DESC);

-- CreateIndex
CREATE INDEX "PlayerInfoMap_mapId_rating_playTime_idx" ON "PlayerInfoMap"("mapId", "rating" DESC NULLS LAST, "playTime" DESC);
