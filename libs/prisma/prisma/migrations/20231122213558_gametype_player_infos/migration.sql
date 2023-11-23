/*
  Warnings:

  - Made the column `name` on table `Map` required. This step will fail if there are existing NULL values in that column.

*/
-- AlterTable
ALTER TABLE `Map` MODIFY `name` VARCHAR(191) NOT NULL;

-- AlterTable
ALTER TABLE `PlayerInfo` ADD COLUMN `gameTypeName` VARCHAR(191) NULL,
    MODIFY `mapId` INTEGER NULL;

-- CreateIndex
CREATE INDEX `PlayerInfo_gameTypeName_idx` ON `PlayerInfo`(`gameTypeName`);
