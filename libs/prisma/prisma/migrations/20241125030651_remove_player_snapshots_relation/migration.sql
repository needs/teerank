/*
  Warnings:

  - You are about to drop the column `gameServerSnapshotId` on the `Player` table. All the data in the column will be lost.

*/
-- DropForeignKey
ALTER TABLE "Player" DROP CONSTRAINT "Player_gameServerSnapshotId_fkey";

-- AlterTable
ALTER TABLE "Player" DROP COLUMN "gameServerSnapshotId";
