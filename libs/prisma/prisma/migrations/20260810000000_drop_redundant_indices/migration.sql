-- On production these indexes are dropped manually with DROP INDEX CONCURRENTLY
-- (which cannot run inside the transaction Prisma wraps migrations in), then this
-- migration is marked applied with `prisma migrate resolve --applied`.
-- See docs/cost-reduction-runbook.md.

-- DropIndex
DROP INDEX IF EXISTS "GameServerSnapshot_createdAt_idx";

-- DropIndex
DROP INDEX IF EXISTS "PlayerInfoMap_playTime_idx";

-- DropIndex
DROP INDEX IF EXISTS "ClanInfoMap_playTime_idx";
