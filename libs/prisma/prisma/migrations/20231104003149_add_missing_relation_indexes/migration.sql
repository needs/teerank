-- CreateIndex
CREATE INDEX `GameServer_masterServerId_idx` ON `GameServer`(`masterServerId`);

-- CreateIndex
CREATE INDEX `GameServerClient_snapshotId_idx` ON `GameServerClient`(`snapshotId`);

-- CreateIndex
CREATE INDEX `GameServerClient_playerName_idx` ON `GameServerClient`(`playerName`);

-- CreateIndex
CREATE INDEX `GameServerSnapshot_gameServerId_idx` ON `GameServerSnapshot`(`gameServerId`);

-- CreateIndex
CREATE INDEX `GameServerSnapshot_mapId_idx` ON `GameServerSnapshot`(`mapId`);

-- CreateIndex
CREATE INDEX `Map_gameTypeName_idx` ON `Map`(`gameTypeName`);

-- CreateIndex
CREATE INDEX `PlayerRatings_mapId_idx` ON `PlayerRatings`(`mapId`);

-- CreateIndex
CREATE INDEX `PlayerRatings_playerName_idx` ON `PlayerRatings`(`playerName`);
