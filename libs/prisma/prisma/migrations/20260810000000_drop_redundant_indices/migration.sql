-- On production these indexes are dropped manually with DROP INDEX CONCURRENTLY,
-- then this migration is marked applied with `prisma migrate resolve --applied`.
--
-- Why CONCURRENTLY can't go in this file: PostgreSQL forbids DROP INDEX
-- CONCURRENTLY inside a transaction block
-- (https://www.postgresql.org/docs/current/sql-dropindex.html), and
-- `prisma migrate deploy` wraps multi-statement migrations like this one in a
-- transaction. Verified on Prisma 5.22: a migration with two
-- DROP INDEX CONCURRENTLY statements fails with SQLSTATE 25001 "DROP INDEX
-- CONCURRENTLY cannot run inside a transaction block" and rolls back, while a
-- single-statement migration is sent unwrapped and succeeds.
-- See docs/cost-reduction-runbook.md.

-- DropIndex
DROP INDEX IF EXISTS "GameServerSnapshot_createdAt_idx";

-- DropIndex
DROP INDEX IF EXISTS "PlayerInfoMap_playTime_idx";

-- DropIndex
DROP INDEX IF EXISTS "ClanInfoMap_playTime_idx";
