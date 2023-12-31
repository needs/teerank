-- CreateEnum
CREATE TYPE "RankMethod" AS ENUM ('ELO');

-- CreateTable
CREATE TABLE "MasterServer" (
    "id" SERIAL NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "address" TEXT NOT NULL,
    "port" INTEGER NOT NULL,

    CONSTRAINT "MasterServer_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "GameServer" (
    "id" SERIAL NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "offlineSince" TIMESTAMP(3),
    "ip" TEXT NOT NULL,
    "port" INTEGER NOT NULL,
    "masterServerId" INTEGER NOT NULL,
    "lastSnapshotId" INTEGER,

    CONSTRAINT "GameServer_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "Player" (
    "name" TEXT NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "clanName" TEXT,
    "playTime" INTEGER NOT NULL DEFAULT 0,

    CONSTRAINT "Player_pkey" PRIMARY KEY ("name")
);

-- CreateTable
CREATE TABLE "GameType" (
    "name" TEXT NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "rankMethod" "RankMethod",

    CONSTRAINT "GameType_pkey" PRIMARY KEY ("name")
);

-- CreateTable
CREATE TABLE "Map" (
    "id" SERIAL NOT NULL,
    "name" TEXT NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "gameTypeName" TEXT NOT NULL,

    CONSTRAINT "Map_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "PlayerInfo" (
    "id" SERIAL NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "playerName" TEXT NOT NULL,
    "mapId" INTEGER,
    "rating" DOUBLE PRECISION,
    "playTime" INTEGER NOT NULL DEFAULT 0,
    "gameTypeName" TEXT,

    CONSTRAINT "PlayerInfo_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "GameServerClient" (
    "id" SERIAL NOT NULL,
    "snapshotId" INTEGER NOT NULL,
    "playerName" TEXT NOT NULL,
    "clanName" TEXT,
    "score" INTEGER NOT NULL,
    "country" INTEGER NOT NULL,
    "inGame" BOOLEAN NOT NULL,

    CONSTRAINT "GameServerClient_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "GameServerSnapshot" (
    "id" SERIAL NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "rankedAt" TIMESTAMP(3),
    "playTimedAt" TIMESTAMP(3),
    "version" TEXT NOT NULL,
    "name" TEXT NOT NULL,
    "mapId" INTEGER NOT NULL,
    "numPlayers" INTEGER NOT NULL,
    "maxPlayers" INTEGER NOT NULL,
    "numClients" INTEGER NOT NULL,
    "maxClients" INTEGER NOT NULL,
    "gameServerId" INTEGER NOT NULL,

    CONSTRAINT "GameServerSnapshot_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "ClanInfo" (
    "id" SERIAL NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "clanName" TEXT NOT NULL,
    "mapId" INTEGER,
    "playTime" INTEGER NOT NULL DEFAULT 0,
    "gameTypeName" TEXT,

    CONSTRAINT "ClanInfo_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "Clan" (
    "name" TEXT NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "playTime" INTEGER NOT NULL DEFAULT 0,

    CONSTRAINT "Clan_pkey" PRIMARY KEY ("name")
);

-- CreateIndex
CREATE UNIQUE INDEX "MasterServer_address_port_key" ON "MasterServer"("address", "port");

-- CreateIndex
CREATE UNIQUE INDEX "GameServer_lastSnapshotId_key" ON "GameServer"("lastSnapshotId");

-- CreateIndex
CREATE UNIQUE INDEX "GameServer_ip_port_key" ON "GameServer"("ip", "port");

-- CreateIndex
CREATE UNIQUE INDEX "Map_name_gameTypeName_key" ON "Map"("name", "gameTypeName");

-- CreateIndex
CREATE UNIQUE INDEX "PlayerInfo_playerName_mapId_key" ON "PlayerInfo"("playerName", "mapId");

-- CreateIndex
CREATE UNIQUE INDEX "PlayerInfo_playerName_gameTypeName_key" ON "PlayerInfo"("playerName", "gameTypeName");

-- CreateIndex
CREATE UNIQUE INDEX "ClanInfo_clanName_mapId_key" ON "ClanInfo"("clanName", "mapId");

-- CreateIndex
CREATE UNIQUE INDEX "ClanInfo_clanName_gameTypeName_key" ON "ClanInfo"("clanName", "gameTypeName");

-- AddForeignKey
ALTER TABLE "GameServer" ADD CONSTRAINT "GameServer_masterServerId_fkey" FOREIGN KEY ("masterServerId") REFERENCES "MasterServer"("id") ON DELETE RESTRICT ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServer" ADD CONSTRAINT "GameServer_lastSnapshotId_fkey" FOREIGN KEY ("lastSnapshotId") REFERENCES "GameServerSnapshot"("id") ON DELETE NO ACTION ON UPDATE NO ACTION;

-- AddForeignKey
ALTER TABLE "Player" ADD CONSTRAINT "Player_clanName_fkey" FOREIGN KEY ("clanName") REFERENCES "Clan"("name") ON DELETE SET NULL ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "Map" ADD CONSTRAINT "Map_gameTypeName_fkey" FOREIGN KEY ("gameTypeName") REFERENCES "GameType"("name") ON DELETE RESTRICT ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "PlayerInfo" ADD CONSTRAINT "PlayerInfo_playerName_fkey" FOREIGN KEY ("playerName") REFERENCES "Player"("name") ON DELETE RESTRICT ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "PlayerInfo" ADD CONSTRAINT "PlayerInfo_mapId_fkey" FOREIGN KEY ("mapId") REFERENCES "Map"("id") ON DELETE SET NULL ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "PlayerInfo" ADD CONSTRAINT "PlayerInfo_gameTypeName_fkey" FOREIGN KEY ("gameTypeName") REFERENCES "GameType"("name") ON DELETE SET NULL ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerClient" ADD CONSTRAINT "GameServerClient_snapshotId_fkey" FOREIGN KEY ("snapshotId") REFERENCES "GameServerSnapshot"("id") ON DELETE RESTRICT ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerClient" ADD CONSTRAINT "GameServerClient_playerName_fkey" FOREIGN KEY ("playerName") REFERENCES "Player"("name") ON DELETE RESTRICT ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerClient" ADD CONSTRAINT "GameServerClient_clanName_fkey" FOREIGN KEY ("clanName") REFERENCES "Clan"("name") ON DELETE SET NULL ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerSnapshot" ADD CONSTRAINT "GameServerSnapshot_mapId_fkey" FOREIGN KEY ("mapId") REFERENCES "Map"("id") ON DELETE RESTRICT ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerSnapshot" ADD CONSTRAINT "GameServerSnapshot_gameServerId_fkey" FOREIGN KEY ("gameServerId") REFERENCES "GameServer"("id") ON DELETE RESTRICT ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ClanInfo" ADD CONSTRAINT "ClanInfo_clanName_fkey" FOREIGN KEY ("clanName") REFERENCES "Clan"("name") ON DELETE RESTRICT ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ClanInfo" ADD CONSTRAINT "ClanInfo_mapId_fkey" FOREIGN KEY ("mapId") REFERENCES "Map"("id") ON DELETE SET NULL ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ClanInfo" ADD CONSTRAINT "ClanInfo_gameTypeName_fkey" FOREIGN KEY ("gameTypeName") REFERENCES "GameType"("name") ON DELETE SET NULL ON UPDATE CASCADE;
