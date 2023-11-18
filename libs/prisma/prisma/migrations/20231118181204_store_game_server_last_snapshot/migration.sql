/*
  Warnings:

  - A unique constraint covering the columns `[lastSnapshotId]` on the table `GameServer` will be added. If there are existing duplicate values, this will fail.

*/
-- AlterTable
ALTER TABLE `GameServer` ADD COLUMN `lastSnapshotId` INTEGER NULL;

-- CreateIndex
CREATE UNIQUE INDEX `GameServer_lastSnapshotId_key` ON `GameServer`(`lastSnapshotId`);

-- CreateIndex
CREATE INDEX `GameServer_lastSnapshotId_idx` ON `GameServer`(`lastSnapshotId`);
