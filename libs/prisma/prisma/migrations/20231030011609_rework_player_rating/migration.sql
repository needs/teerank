/*
  Warnings:

  - You are about to drop the `PlayerElo` table. If the table is not empty, all the data it contains will be lost.

*/
-- AlterTable
ALTER TABLE `GameServerSnapshot` ADD COLUMN `rankedAt` DATETIME(3) NULL;

-- AlterTable
ALTER TABLE `GameType` ADD COLUMN `rankMethod` ENUM('ELO') NULL;

-- DropTable
DROP TABLE `PlayerElo`;

-- CreateTable
CREATE TABLE `PlayerRatings` (
    `id` INTEGER NOT NULL AUTO_INCREMENT,
    `createdAt` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `updatedAt` DATETIME(3) NOT NULL,
    `playerName` VARCHAR(191) NOT NULL,
    `mapId` INTEGER NOT NULL,
    `rating` DOUBLE NOT NULL,

    UNIQUE INDEX `PlayerRatings_playerName_mapId_key`(`playerName`, `mapId`),
    PRIMARY KEY (`id`)
) DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
