/*
  Warnings:

  - You are about to drop the column `playerId` on the `GameServerClient` table. All the data in the column will be lost.
  - The primary key for the `GameType` table will be changed. If it partially fails, the table could be left without primary key constraint.
  - You are about to drop the column `id` on the `GameType` table. All the data in the column will be lost.
  - You are about to drop the column `gameTypeId` on the `Map` table. All the data in the column will be lost.
  - The primary key for the `Player` table will be changed. If it partially fails, the table could be left without primary key constraint.
  - You are about to drop the column `id` on the `Player` table. All the data in the column will be lost.
  - You are about to drop the column `playerId` on the `PlayerElo` table. All the data in the column will be lost.
  - A unique constraint covering the columns `[name,gameTypeName]` on the table `Map` will be added. If there are existing duplicate values, this will fail.
  - A unique constraint covering the columns `[playerName,mapId]` on the table `PlayerElo` will be added. If there are existing duplicate values, this will fail.
  - Added the required column `playerName` to the `GameServerClient` table without a default value. This is not possible if the table is not empty.
  - Added the required column `gameTypeName` to the `Map` table without a default value. This is not possible if the table is not empty.
  - Added the required column `playerName` to the `PlayerElo` table without a default value. This is not possible if the table is not empty.

*/
-- DropIndex
DROP INDEX `GameType_name_key` ON `GameType`;

-- DropIndex
DROP INDEX `Map_name_gameTypeId_key` ON `Map`;

-- DropIndex
DROP INDEX `Player_name_key` ON `Player`;

-- DropIndex
DROP INDEX `PlayerElo_playerId_mapId_key` ON `PlayerElo`;

-- AlterTable
ALTER TABLE `GameServerClient` DROP COLUMN `playerId`,
    ADD COLUMN `playerName` VARCHAR(191) NOT NULL;

-- AlterTable
ALTER TABLE `GameType` DROP PRIMARY KEY,
    DROP COLUMN `id`,
    ADD PRIMARY KEY (`name`);

-- AlterTable
ALTER TABLE `Map` DROP COLUMN `gameTypeId`,
    ADD COLUMN `gameTypeName` VARCHAR(191) NOT NULL;

-- AlterTable
ALTER TABLE `Player` DROP PRIMARY KEY,
    DROP COLUMN `id`,
    ADD PRIMARY KEY (`name`);

-- AlterTable
ALTER TABLE `PlayerElo` DROP COLUMN `playerId`,
    ADD COLUMN `playerName` VARCHAR(191) NOT NULL;

-- CreateIndex
CREATE UNIQUE INDEX `Map_name_gameTypeName_key` ON `Map`(`name`, `gameTypeName`);

-- CreateIndex
CREATE UNIQUE INDEX `PlayerElo_playerName_mapId_key` ON `PlayerElo`(`playerName`, `mapId`);
