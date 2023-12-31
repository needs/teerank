/*
  Warnings:

  - You are about to drop the column `clanInfoGameTypeId` on the `GameServerClient` table. All the data in the column will be lost.
  - You are about to drop the column `clanInfoMapId` on the `GameServerClient` table. All the data in the column will be lost.
  - You are about to drop the column `playerInfoGameTypeId` on the `GameServerClient` table. All the data in the column will be lost.
  - You are about to drop the column `playerInfoMapId` on the `GameServerClient` table. All the data in the column will be lost.

*/
-- DropForeignKey
ALTER TABLE "GameServerClient" DROP CONSTRAINT "GameServerClient_clanInfoGameTypeId_fkey";

-- DropForeignKey
ALTER TABLE "GameServerClient" DROP CONSTRAINT "GameServerClient_clanInfoMapId_fkey";

-- DropForeignKey
ALTER TABLE "GameServerClient" DROP CONSTRAINT "GameServerClient_playerInfoGameTypeId_fkey";

-- DropForeignKey
ALTER TABLE "GameServerClient" DROP CONSTRAINT "GameServerClient_playerInfoMapId_fkey";

-- AlterTable
ALTER TABLE "GameServerClient" DROP COLUMN "clanInfoGameTypeId",
DROP COLUMN "clanInfoMapId",
DROP COLUMN "playerInfoGameTypeId",
DROP COLUMN "playerInfoMapId";
