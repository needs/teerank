/*
  Warnings:

  - You are about to drop the column `name` on the `GameServerClient` table. All the data in the column will be lost.
  - You are about to drop the column `mapName` on the `GameServerSnapshot` table. All the data in the column will be lost.
  - The primary key for the `GameType` table will be changed. If it partially fails, the table could be left without primary key constraint.
  - The primary key for the `Map` table will be changed. If it partially fails, the table could be left without primary key constraint.
  - You are about to drop the column `gameTypeName` on the `Map` table. All the data in the column will be lost.
  - The primary key for the `Player` table will be changed. If it partially fails, the table could be left without primary key constraint.
  - You are about to drop the column `mapName` on the `PlayerElo` table. All the data in the column will be lost.
  - You are about to drop the column `name` on the `PlayerElo` table. All the data in the column will be lost.
  - A unique constraint covering the columns `[name]` on the table `GameType` will be added. If there are existing duplicate values, this will fail.
  - A unique constraint covering the columns `[name,gameTypeId]` on the table `Map` will be added. If there are existing duplicate values, this will fail.
  - A unique constraint covering the columns `[name]` on the table `Player` will be added. If there are existing duplicate values, this will fail.
  - A unique constraint covering the columns `[playerId,mapId]` on the table `PlayerElo` will be added. If there are existing duplicate values, this will fail.
  - Added the required column `playerId` to the `GameServerClient` table without a default value. This is not possible if the table is not empty.
  - Added the required column `mapId` to the `GameServerSnapshot` table without a default value. This is not possible if the table is not empty.
  - Added the required column `id` to the `GameType` table without a default value. This is not possible if the table is not empty.
  - Added the required column `gameTypeId` to the `Map` table without a default value. This is not possible if the table is not empty.
  - Added the required column `id` to the `Map` table without a default value. This is not possible if the table is not empty.
  - Added the required column `id` to the `Player` table without a default value. This is not possible if the table is not empty.
  - Added the required column `mapId` to the `PlayerElo` table without a default value. This is not possible if the table is not empty.
  - Added the required column `playerId` to the `PlayerElo` table without a default value. This is not possible if the table is not empty.

*/
-- AlterTable
ALTER TABLE `GameServerClient` DROP COLUMN `name`,
    ADD COLUMN `playerId` INTEGER NOT NULL;

-- AlterTable
ALTER TABLE `GameServerSnapshot` DROP COLUMN `mapName`,
    ADD COLUMN `mapId` INTEGER NOT NULL;

-- AlterTable
ALTER TABLE `GameType` DROP PRIMARY KEY,
    ADD COLUMN `id` INTEGER NOT NULL AUTO_INCREMENT,
    ADD PRIMARY KEY (`id`);

-- AlterTable
ALTER TABLE `Map` DROP PRIMARY KEY,
    DROP COLUMN `gameTypeName`,
    ADD COLUMN `gameTypeId` VARCHAR(191) NOT NULL,
    ADD COLUMN `id` INTEGER NOT NULL AUTO_INCREMENT,
    ADD PRIMARY KEY (`id`);

-- AlterTable
ALTER TABLE `Player` DROP PRIMARY KEY,
    ADD COLUMN `id` INTEGER NOT NULL AUTO_INCREMENT,
    ADD PRIMARY KEY (`id`);

-- AlterTable
ALTER TABLE `PlayerElo` DROP COLUMN `mapName`,
    DROP COLUMN `name`,
    ADD COLUMN `mapId` INTEGER NOT NULL,
    ADD COLUMN `playerId` INTEGER NOT NULL;

-- CreateIndex
CREATE UNIQUE INDEX `GameType_name_key` ON `GameType`(`name`);

-- CreateIndex
CREATE UNIQUE INDEX `Map_name_gameTypeId_key` ON `Map`(`name`, `gameTypeId`);

-- CreateIndex
CREATE UNIQUE INDEX `Player_name_key` ON `Player`(`name`);

-- CreateIndex
CREATE UNIQUE INDEX `PlayerElo_playerId_mapId_key` ON `PlayerElo`(`playerId`, `mapId`);
