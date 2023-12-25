/*
  Warnings:

  - You are about to drop the `Job` table. If the table is not empty, all the data it contains will be lost.

*/
-- AlterTable
ALTER TABLE "GameServer" ADD COLUMN     "pollBatchUuid" UUID,
ADD COLUMN     "pollingStartedAt" TIMESTAMP(3);

-- AlterTable
ALTER TABLE "GameServerSnapshot" ADD COLUMN     "playTimingStartedAt" TIMESTAMP(3),
ADD COLUMN     "rankingStartedAt" TIMESTAMP(3);

-- AlterTable
ALTER TABLE "MasterServer" ADD COLUMN     "pollingStartedAt" TIMESTAMP(3);

-- DropTable
DROP TABLE "Job";

-- DropEnum
DROP TYPE "JobType";

-- CreateIndex
CREATE INDEX "GameServerSnapshot_rankingStartedAt_idx" ON "GameServerSnapshot"("rankingStartedAt");

-- CreateIndex
CREATE INDEX "GameServerSnapshot_playTimingStartedAt_idx" ON "GameServerSnapshot"("playTimingStartedAt");

-- CreateIndex
CREATE INDEX "GameServerSnapshot_createdAt_rankingStartedAt_idx" ON "GameServerSnapshot"("createdAt" DESC, "rankingStartedAt");

-- CreateIndex
CREATE INDEX "GameServerSnapshot_createdAt_playTimingStartedAt_idx" ON "GameServerSnapshot"("createdAt" DESC, "playTimingStartedAt");
