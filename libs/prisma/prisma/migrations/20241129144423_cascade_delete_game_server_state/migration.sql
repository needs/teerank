-- DropForeignKey
ALTER TABLE "GameServerState" DROP CONSTRAINT "GameServerState_gameServerId_fkey";

-- AddForeignKey
ALTER TABLE "GameServerState" ADD CONSTRAINT "GameServerState_gameServerId_fkey" FOREIGN KEY ("gameServerId") REFERENCES "GameServer"("id") ON DELETE CASCADE ON UPDATE CASCADE;
