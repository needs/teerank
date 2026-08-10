-- GameServerStateClient.id burns ~400k sequence values a day from the state
-- delete/recreate cycle and would eventually overflow int4. The table holds
-- under a thousand rows, so the rewrite is sub-second.
-- AlterTable
ALTER TABLE "GameServerStateClient" ALTER COLUMN "id" SET DATA TYPE BIGINT;

-- Prisma doesn't auto-index relation scalars; /status counts by masterServer
-- and was seq-scanning GameServer on every hit.
-- CreateIndex
CREATE INDEX "GameServer_masterServerId_idx" ON "GameServer"("masterServerId");
