-- RenameIndex
ALTER TABLE `PlayerInfo` RENAME INDEX `PlayerRatings_mapId_idx` TO `PlayerInfo_mapId_idx`;

-- RenameIndex
ALTER TABLE `PlayerInfo` RENAME INDEX `PlayerRatings_playerName_idx` TO `PlayerInfo_playerName_idx`;

-- RenameIndex
ALTER TABLE `PlayerInfo` RENAME INDEX `PlayerRatings_playerName_mapId_key` TO `PlayerInfo_playerName_mapId_key`;
