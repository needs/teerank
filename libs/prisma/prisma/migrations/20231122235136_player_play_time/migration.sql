/*
  Warnings:

  - A unique constraint covering the columns `[playerName,gameTypeName]` on the table `PlayerInfo` will be added. If there are existing duplicate values, this will fail.

*/
-- AlterTable
ALTER TABLE `Player` ADD COLUMN `playTime` INTEGER NOT NULL DEFAULT 0;

-- CreateIndex
CREATE UNIQUE INDEX `PlayerInfo_playerName_gameTypeName_key` ON `PlayerInfo`(`playerName`, `gameTypeName`);
