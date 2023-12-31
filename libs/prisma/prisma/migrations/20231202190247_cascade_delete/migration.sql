-- AlterEnum
ALTER TYPE "RankMethod" ADD VALUE 'TIME';

-- DropForeignKey
ALTER TABLE "ClanInfo" DROP CONSTRAINT "ClanInfo_clanName_fkey";

-- DropForeignKey
ALTER TABLE "ClanInfo" DROP CONSTRAINT "ClanInfo_gameTypeName_fkey";

-- DropForeignKey
ALTER TABLE "ClanInfo" DROP CONSTRAINT "ClanInfo_mapId_fkey";

-- DropForeignKey
ALTER TABLE "GameServerClient" DROP CONSTRAINT "GameServerClient_clanName_fkey";

-- DropForeignKey
ALTER TABLE "GameServerClient" DROP CONSTRAINT "GameServerClient_playerName_fkey";

-- DropForeignKey
ALTER TABLE "GameServerClient" DROP CONSTRAINT "GameServerClient_snapshotId_fkey";

-- DropForeignKey
ALTER TABLE "GameServerSnapshot" DROP CONSTRAINT "GameServerSnapshot_gameServerId_fkey";

-- DropForeignKey
ALTER TABLE "GameServerSnapshot" DROP CONSTRAINT "GameServerSnapshot_mapId_fkey";

-- DropForeignKey
ALTER TABLE "Map" DROP CONSTRAINT "Map_gameTypeName_fkey";

-- DropForeignKey
ALTER TABLE "PlayerInfo" DROP CONSTRAINT "PlayerInfo_gameTypeName_fkey";

-- DropForeignKey
ALTER TABLE "PlayerInfo" DROP CONSTRAINT "PlayerInfo_mapId_fkey";

-- DropForeignKey
ALTER TABLE "PlayerInfo" DROP CONSTRAINT "PlayerInfo_playerName_fkey";

-- DropForeignKey
ALTER TABLE "TaskRun" DROP CONSTRAINT "TaskRun_taskName_fkey";

-- AddForeignKey
ALTER TABLE "Map" ADD CONSTRAINT "Map_gameTypeName_fkey" FOREIGN KEY ("gameTypeName") REFERENCES "GameType"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "PlayerInfo" ADD CONSTRAINT "PlayerInfo_playerName_fkey" FOREIGN KEY ("playerName") REFERENCES "Player"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "PlayerInfo" ADD CONSTRAINT "PlayerInfo_mapId_fkey" FOREIGN KEY ("mapId") REFERENCES "Map"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "PlayerInfo" ADD CONSTRAINT "PlayerInfo_gameTypeName_fkey" FOREIGN KEY ("gameTypeName") REFERENCES "GameType"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerClient" ADD CONSTRAINT "GameServerClient_snapshotId_fkey" FOREIGN KEY ("snapshotId") REFERENCES "GameServerSnapshot"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerClient" ADD CONSTRAINT "GameServerClient_playerName_fkey" FOREIGN KEY ("playerName") REFERENCES "Player"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerClient" ADD CONSTRAINT "GameServerClient_clanName_fkey" FOREIGN KEY ("clanName") REFERENCES "Clan"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerSnapshot" ADD CONSTRAINT "GameServerSnapshot_mapId_fkey" FOREIGN KEY ("mapId") REFERENCES "Map"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "GameServerSnapshot" ADD CONSTRAINT "GameServerSnapshot_gameServerId_fkey" FOREIGN KEY ("gameServerId") REFERENCES "GameServer"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ClanInfo" ADD CONSTRAINT "ClanInfo_clanName_fkey" FOREIGN KEY ("clanName") REFERENCES "Clan"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ClanInfo" ADD CONSTRAINT "ClanInfo_mapId_fkey" FOREIGN KEY ("mapId") REFERENCES "Map"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ClanInfo" ADD CONSTRAINT "ClanInfo_gameTypeName_fkey" FOREIGN KEY ("gameTypeName") REFERENCES "GameType"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "TaskRun" ADD CONSTRAINT "TaskRun_taskName_fkey" FOREIGN KEY ("taskName") REFERENCES "Task"("name") ON DELETE CASCADE ON UPDATE CASCADE;
