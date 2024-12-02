/*
  Warnings:

  - You are about to drop the column `countedAt` on the `Map` table. All the data in the column will be lost.

*/
-- DropIndex
DROP INDEX "GameServerSnapshot_version_idx";

-- DropIndex
DROP INDEX "Map_countedAt_idx";

-- AlterTable
ALTER TABLE "Map" DROP COLUMN "countedAt";

-- CreateIndex
CREATE INDEX "Map_createdAt_idx" ON "Map"("createdAt");
