-- AlterTable
ALTER TABLE "GameServer" ADD COLUMN     "polledAt" TIMESTAMP(3);

-- AlterTable
ALTER TABLE "Job" ADD COLUMN     "startedAt" TIMESTAMP(3);

-- AlterTable
ALTER TABLE "MasterServer" ADD COLUMN     "polledAt" TIMESTAMP(3);
