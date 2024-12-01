/*
  Warnings:

  - You are about to drop the column `rankedAt` on the `GameServerSnapshot` table. All the data in the column will be lost.
  - You are about to drop the column `rankingStartedAt` on the `GameServerSnapshot` table. All the data in the column will be lost.

*/
-- DropIndex
DROP INDEX "GameServerSnapshot_rankedAt_idx";

-- DropIndex
DROP INDEX "GameServerSnapshot_rankingStartedAt_idx";

-- AlterTable
ALTER TABLE "GameServerSnapshot" DROP COLUMN "rankedAt",
DROP COLUMN "rankingStartedAt";
