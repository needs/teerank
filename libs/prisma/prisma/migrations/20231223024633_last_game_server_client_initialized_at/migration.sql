/*
  Warnings:

  - A unique constraint covering the columns `[lastGameServerClientId]` on the table `Player` will be added. If there are existing duplicate values, this will fail.

*/
-- AlterEnum
ALTER TYPE "JobType" ADD VALUE 'INITIALIZE_PLAYER_LAST_GAME_SERVER_CLIENT';

-- AlterTable
ALTER TABLE "Player" ADD COLUMN     "gameServerSnapshotId" INTEGER,
ADD COLUMN     "lastGameServerClientId" INTEGER,
ADD COLUMN     "lastGameServerClientInitializedAt" TIMESTAMP(3) DEFAULT CURRENT_TIMESTAMP;

-- CreateIndex
CREATE UNIQUE INDEX "Player_lastGameServerClientId_key" ON "Player"("lastGameServerClientId");

-- CreateIndex
CREATE INDEX "Player_createdAt_lastGameServerClientInitializedAt_idx" ON "Player"("createdAt" DESC, "lastGameServerClientInitializedAt");

-- CreateIndex
CREATE INDEX "Player_lastGameServerClientInitializedAt_idx" ON "Player"("lastGameServerClientInitializedAt");

-- AddForeignKey
ALTER TABLE "Player" ADD CONSTRAINT "Player_lastGameServerClientId_fkey" FOREIGN KEY ("lastGameServerClientId") REFERENCES "GameServerClient"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "Player" ADD CONSTRAINT "Player_gameServerSnapshotId_fkey" FOREIGN KEY ("gameServerSnapshotId") REFERENCES "GameServerSnapshot"("id") ON DELETE SET NULL ON UPDATE CASCADE;
