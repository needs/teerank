-- CreateIndex
CREATE INDEX "Clan_name_idx" ON "Clan" USING GIN ("name" gin_trgm_ops);

-- CreateIndex
CREATE INDEX "GameServerState_name_idx" ON "GameServerState" USING GIN ("name" gin_trgm_ops);

-- CreateIndex
CREATE INDEX "Player_name_idx" ON "Player" USING GIN ("name" gin_trgm_ops);
