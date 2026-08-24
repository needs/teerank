-- Single statement on purpose: CONCURRENTLY cannot run inside the
-- transaction Prisma wraps multi-statement migrations in.
CREATE INDEX CONCURRENTLY "GameServerClient_playerName_id_idx" ON "GameServerClient"("playerName", "id");
