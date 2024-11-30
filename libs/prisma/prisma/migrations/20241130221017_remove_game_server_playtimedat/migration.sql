/*
  Warnings:

  - You are about to drop the column `gameServerPlayTimedAt` on the `GameServerSnapshot` table. All the data in the column will be lost.
  - You are about to drop the column `gameServerPlayTimingStartedAt` on the `GameServerSnapshot` table. All the data in the column will be lost.
  - You are about to drop the column `playTimedAt` on the `GameServerSnapshot` table. All the data in the column will be lost.
  - You are about to drop the column `playTimingStartedAt` on the `GameServerSnapshot` table. All the data in the column will be lost.

*/
-- DropIndex
DROP INDEX "GameServerSnapshot_createdAt_playTimingStartedAt_idx";

-- DropIndex
DROP INDEX "GameServerSnapshot_gameServerPlayTimedAt_idx";

-- DropIndex
DROP INDEX "GameServerSnapshot_gameServerPlayTimingStartedAt_idx";

-- DropIndex
DROP INDEX "GameServerSnapshot_playTimedAt_idx";

-- DropIndex
DROP INDEX "GameServerSnapshot_playTimingStartedAt_idx";

-- AlterTable
ALTER TABLE "GameServerSnapshot" DROP COLUMN "gameServerPlayTimedAt",
DROP COLUMN "gameServerPlayTimingStartedAt",
DROP COLUMN "playTimedAt",
DROP COLUMN "playTimingStartedAt";
