/*
  Warnings:

  - You are about to drop the column `lastGameServerClientInitializedAt` on the `Player` table. All the data in the column will be lost.

*/
-- DropIndex
DROP INDEX "Player_createdAt_lastGameServerClientInitializedAt_idx";

-- DropIndex
DROP INDEX "Player_lastGameServerClientInitializedAt_idx";

-- AlterTable
ALTER TABLE "Player" DROP COLUMN "lastGameServerClientInitializedAt";
