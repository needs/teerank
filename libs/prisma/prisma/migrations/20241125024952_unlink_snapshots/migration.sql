/*
  Warnings:

  - You are about to drop the column `lastSnapshotId` on the `GameServer` table. All the data in the column will be lost.
  - You are about to drop the column `offlineSince` on the `GameServer` table. All the data in the column will be lost.
  - You are about to drop the column `lastGameServerClientId` on the `Player` table. All the data in the column will be lost.
  - A unique constraint covering the columns `[gameServerStateId]` on the table `GameServer` will be added. If there are existing duplicate values, this will fail.

*/
-- DropForeignKey
ALTER TABLE "GameServer" DROP CONSTRAINT "GameServer_lastSnapshotId_fkey";

-- DropForeignKey
ALTER TABLE "Player" DROP CONSTRAINT "Player_lastGameServerClientId_fkey";

-- DropIndex
DROP INDEX "GameServer_lastSnapshotId_key";

-- DropIndex
DROP INDEX "Player_lastGameServerClientId_key";

-- AlterTable
ALTER TABLE "GameServer" DROP COLUMN "lastSnapshotId",
DROP COLUMN "offlineSince",
ADD COLUMN     "gameServerStateId" INTEGER,
ADD COLUMN     "lastSeenAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP;

-- AlterTable
ALTER TABLE "Player" DROP COLUMN "lastGameServerClientId",
ADD COLUMN     "lastSeenAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP;

-- CreateTable
CREATE TABLE "GameServerState" (
    "id" SERIAL NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "version" TEXT NOT NULL,
    "name" TEXT NOT NULL,
    "mapId" INTEGER NOT NULL,
    "numPlayers" INTEGER NOT NULL,
    "maxPlayers" INTEGER NOT NULL,
    "numClients" INTEGER NOT NULL,
    "maxClients" INTEGER NOT NULL,

    CONSTRAINT "GameServerState_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "GameServerStateClient" (
    "id" SERIAL NOT NULL,
    "gameServerStateId" INTEGER NOT NULL,
    "playerName" TEXT NOT NULL,
    "clanName" TEXT,
    "score" INTEGER NOT NULL,
    "country" INTEGER NOT NULL,
    "inGame" BOOLEAN NOT NULL,

    CONSTRAINT "GameServerStateClient_pkey" PRIMARY KEY ("id")
);

-- CreateIndex
CREATE INDEX "GameServerStateClient_gameServerStateId_idx" ON "GameServerStateClient"("gameServerStateId");

-- CreateIndex
CREATE INDEX "GameServerStateClient_score_inGame_idx" ON "GameServerStateClient"("score", "inGame");

-- CreateIndex
CREATE INDEX "GameServerStateClient_playerName_idx" ON "GameServerStateClient"("playerName");

-- CreateIndex
CREATE UNIQUE INDEX "GameServer_gameServerStateId_key" ON "GameServer"("gameServerStateId");

-- AddForeignKey
ALTER TABLE "GameServer" ADD CONSTRAINT "GameServer_gameServerStateId_fkey" FOREIGN KEY ("gameServerStateId") REFERENCES "GameServerState"("id") ON DELETE SET NULL ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerState" ADD CONSTRAINT "GameServerState_mapId_fkey" FOREIGN KEY ("mapId") REFERENCES "Map"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerStateClient" ADD CONSTRAINT "GameServerStateClient_gameServerStateId_fkey" FOREIGN KEY ("gameServerStateId") REFERENCES "GameServerState"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerStateClient" ADD CONSTRAINT "GameServerStateClient_playerName_fkey" FOREIGN KEY ("playerName") REFERENCES "Player"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerStateClient" ADD CONSTRAINT "GameServerStateClient_clanName_fkey" FOREIGN KEY ("clanName") REFERENCES "Clan"("name") ON DELETE CASCADE ON UPDATE CASCADE;
