/*
  Warnings:

  - You are about to drop the column `countedAt` on the `GameType` table. All the data in the column will be lost.

*/
-- DropIndex
DROP INDEX "GameType_countedAt_idx";

-- AlterTable
ALTER TABLE "GameType" DROP COLUMN "countedAt";

-- CreateIndex
CREATE INDEX "GameType_createdAt_idx" ON "GameType"("createdAt");
