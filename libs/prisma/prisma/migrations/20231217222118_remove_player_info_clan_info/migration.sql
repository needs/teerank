/*
  Warnings:

  - You are about to drop the `ClanInfo` table. If the table is not empty, all the data it contains will be lost.
  - You are about to drop the `PlayerInfo` table. If the table is not empty, all the data it contains will be lost.

*/
-- DropForeignKey
ALTER TABLE "ClanInfo" DROP CONSTRAINT "ClanInfo_clanName_fkey";

-- DropForeignKey
ALTER TABLE "ClanInfo" DROP CONSTRAINT "ClanInfo_gameTypeName_fkey";

-- DropForeignKey
ALTER TABLE "ClanInfo" DROP CONSTRAINT "ClanInfo_mapId_fkey";

-- DropForeignKey
ALTER TABLE "PlayerInfo" DROP CONSTRAINT "PlayerInfo_gameTypeName_fkey";

-- DropForeignKey
ALTER TABLE "PlayerInfo" DROP CONSTRAINT "PlayerInfo_mapId_fkey";

-- DropForeignKey
ALTER TABLE "PlayerInfo" DROP CONSTRAINT "PlayerInfo_playerName_fkey";

-- DropTable
DROP TABLE "ClanInfo";

-- DropTable
DROP TABLE "PlayerInfo";
