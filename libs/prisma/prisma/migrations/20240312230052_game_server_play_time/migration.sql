-- AlterTable
ALTER TABLE "GameServer" ADD COLUMN     "playTime" BIGINT NOT NULL DEFAULT 0;

-- AlterTable
ALTER TABLE "GameServerSnapshot" ADD COLUMN     "gameServerPlayTimedAt" TIMESTAMP(3),
ADD COLUMN     "gameServerPlayTimingStartedAt" TIMESTAMP(3);
