/*
  Warnings:

  - You are about to drop the column `gameServerStateId` on the `GameServer` table. All the data in the column will be lost.
  - A unique constraint covering the columns `[gameServerId]` on the table `GameServerState` will be added. If there are existing duplicate values, this will fail.
  - Added the required column `gameServerId` to the `GameServerState` table without a default value. This is not possible if the table is not empty.

*/
-- DropForeignKey
ALTER TABLE "GameServer" DROP CONSTRAINT "GameServer_gameServerStateId_fkey";

-- DropIndex
DROP INDEX "GameServer_gameServerStateId_key";

-- AlterTable
ALTER TABLE "GameServer" DROP COLUMN "gameServerStateId";

-- AlterTable
ALTER TABLE "GameServerState" ADD COLUMN     "gameServerId" INTEGER NOT NULL;

-- CreateIndex
CREATE UNIQUE INDEX "GameServerState_gameServerId_key" ON "GameServerState"("gameServerId");

-- AddForeignKey
ALTER TABLE "GameServerState" ADD CONSTRAINT "GameServerState_gameServerId_fkey" FOREIGN KEY ("gameServerId") REFERENCES "GameServer"("id") ON DELETE RESTRICT ON UPDATE CASCADE;
