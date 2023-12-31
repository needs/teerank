-- AlterTable
ALTER TABLE "GameServerClient" ADD COLUMN     "clanInfoGameTypeId" INTEGER,
ADD COLUMN     "clanInfoMapId" INTEGER,
ADD COLUMN     "playerInfoGameTypeId" INTEGER,
ADD COLUMN     "playerInfoMapId" INTEGER;

-- CreateTable
CREATE TABLE "PlayerInfoGameType" (
    "id" SERIAL NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "playerName" TEXT NOT NULL,
    "rating" DOUBLE PRECISION,
    "playTime" INTEGER NOT NULL DEFAULT 0,
    "gameTypeName" TEXT NOT NULL,

    CONSTRAINT "PlayerInfoGameType_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "PlayerInfoMap" (
    "id" SERIAL NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "playerName" TEXT NOT NULL,
    "rating" DOUBLE PRECISION,
    "playTime" INTEGER NOT NULL DEFAULT 0,
    "mapId" INTEGER NOT NULL,

    CONSTRAINT "PlayerInfoMap_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "ClanInfoGameType" (
    "id" SERIAL NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "clanName" TEXT NOT NULL,
    "playTime" INTEGER NOT NULL DEFAULT 0,
    "gameTypeName" TEXT NOT NULL,

    CONSTRAINT "ClanInfoGameType_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "ClanInfoMap" (
    "id" SERIAL NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "clanName" TEXT NOT NULL,
    "playTime" INTEGER NOT NULL DEFAULT 0,
    "mapId" INTEGER NOT NULL,

    CONSTRAINT "ClanInfoMap_pkey" PRIMARY KEY ("id")
);

-- CreateIndex
CREATE INDEX "PlayerInfoGameType_playTime_idx" ON "PlayerInfoGameType"("playTime" DESC);

-- CreateIndex
CREATE UNIQUE INDEX "PlayerInfoGameType_playerName_gameTypeName_key" ON "PlayerInfoGameType"("playerName", "gameTypeName");

-- CreateIndex
CREATE INDEX "PlayerInfoMap_playTime_idx" ON "PlayerInfoMap"("playTime" DESC);

-- CreateIndex
CREATE UNIQUE INDEX "PlayerInfoMap_playerName_mapId_key" ON "PlayerInfoMap"("playerName", "mapId");

-- CreateIndex
CREATE INDEX "ClanInfoGameType_playTime_idx" ON "ClanInfoGameType"("playTime" DESC);

-- CreateIndex
CREATE UNIQUE INDEX "ClanInfoGameType_clanName_gameTypeName_key" ON "ClanInfoGameType"("clanName", "gameTypeName");

-- CreateIndex
CREATE INDEX "ClanInfoMap_playTime_idx" ON "ClanInfoMap"("playTime" DESC);

-- CreateIndex
CREATE UNIQUE INDEX "ClanInfoMap_clanName_mapId_key" ON "ClanInfoMap"("clanName", "mapId");

-- AddForeignKey
ALTER TABLE "PlayerInfoGameType" ADD CONSTRAINT "PlayerInfoGameType_playerName_fkey" FOREIGN KEY ("playerName") REFERENCES "Player"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "PlayerInfoGameType" ADD CONSTRAINT "PlayerInfoGameType_gameTypeName_fkey" FOREIGN KEY ("gameTypeName") REFERENCES "GameType"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "PlayerInfoMap" ADD CONSTRAINT "PlayerInfoMap_playerName_fkey" FOREIGN KEY ("playerName") REFERENCES "Player"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "PlayerInfoMap" ADD CONSTRAINT "PlayerInfoMap_mapId_fkey" FOREIGN KEY ("mapId") REFERENCES "Map"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerClient" ADD CONSTRAINT "GameServerClient_playerInfoGameTypeId_fkey" FOREIGN KEY ("playerInfoGameTypeId") REFERENCES "PlayerInfoGameType"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerClient" ADD CONSTRAINT "GameServerClient_playerInfoMapId_fkey" FOREIGN KEY ("playerInfoMapId") REFERENCES "PlayerInfoMap"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerClient" ADD CONSTRAINT "GameServerClient_clanInfoGameTypeId_fkey" FOREIGN KEY ("clanInfoGameTypeId") REFERENCES "ClanInfoGameType"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerClient" ADD CONSTRAINT "GameServerClient_clanInfoMapId_fkey" FOREIGN KEY ("clanInfoMapId") REFERENCES "ClanInfoMap"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ClanInfoGameType" ADD CONSTRAINT "ClanInfoGameType_clanName_fkey" FOREIGN KEY ("clanName") REFERENCES "Clan"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ClanInfoGameType" ADD CONSTRAINT "ClanInfoGameType_gameTypeName_fkey" FOREIGN KEY ("gameTypeName") REFERENCES "GameType"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ClanInfoMap" ADD CONSTRAINT "ClanInfoMap_clanName_fkey" FOREIGN KEY ("clanName") REFERENCES "Clan"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ClanInfoMap" ADD CONSTRAINT "ClanInfoMap_mapId_fkey" FOREIGN KEY ("mapId") REFERENCES "Map"("id") ON DELETE CASCADE ON UPDATE CASCADE;
