/*
  Warnings:

  - You are about to drop the column `clan` on the `GameServerClient` table. All the data in the column will be lost.

*/
-- AlterTable
ALTER TABLE `GameServerClient` DROP COLUMN `clan`,
    ADD COLUMN `clanName` VARCHAR(191) NULL;

-- AlterTable
ALTER TABLE `Player` ADD COLUMN `clanName` VARCHAR(191) NULL;

-- CreateTable
CREATE TABLE `ClanInfo` (
    `id` INTEGER NOT NULL AUTO_INCREMENT,
    `createdAt` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `updatedAt` DATETIME(3) NOT NULL,
    `clanName` VARCHAR(191) NOT NULL,
    `mapId` INTEGER NULL,
    `playTime` INTEGER NOT NULL DEFAULT 0,
    `gameTypeName` VARCHAR(191) NULL,

    INDEX `ClanInfo_mapId_idx`(`mapId`),
    INDEX `ClanInfo_clanName_idx`(`clanName`),
    INDEX `ClanInfo_gameTypeName_idx`(`gameTypeName`),
    UNIQUE INDEX `ClanInfo_clanName_mapId_key`(`clanName`, `mapId`),
    UNIQUE INDEX `ClanInfo_clanName_gameTypeName_key`(`clanName`, `gameTypeName`),
    PRIMARY KEY (`id`)
) DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- CreateTable
CREATE TABLE `Clan` (
    `name` VARCHAR(191) NOT NULL,
    `createdAt` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `updatedAt` DATETIME(3) NOT NULL,
    `playTime` INTEGER NOT NULL DEFAULT 0,

    INDEX `Clan_name_idx`(`name`),
    PRIMARY KEY (`name`)
) DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- CreateIndex
CREATE INDEX `GameServerClient_clanName_idx` ON `GameServerClient`(`clanName`);

-- CreateIndex
CREATE INDEX `Player_clanName_idx` ON `Player`(`clanName`);
