/*
  Warnings:

  - You are about to drop the column `pollBatchUuid` on the `GameServer` table. All the data in the column will be lost.
  - You are about to drop the column `polledAt` on the `GameServer` table. All the data in the column will be lost.
  - You are about to drop the column `pollingStartedAt` on the `GameServer` table. All the data in the column will be lost.
  - You are about to drop the column `polledAt` on the `MasterServer` table. All the data in the column will be lost.
  - You are about to drop the column `pollingStartedAt` on the `MasterServer` table. All the data in the column will be lost.

*/
-- DropIndex
DROP INDEX "GameServer_polledAt_idx";

-- AlterTable
ALTER TABLE "GameServer" DROP COLUMN "pollBatchUuid",
DROP COLUMN "polledAt",
DROP COLUMN "pollingStartedAt";

-- AlterTable
ALTER TABLE "MasterServer" DROP COLUMN "polledAt",
DROP COLUMN "pollingStartedAt";
