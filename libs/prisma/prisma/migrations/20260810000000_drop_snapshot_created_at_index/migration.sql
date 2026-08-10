-- ⚠️ This migration must stay a SINGLE statement. `prisma migrate deploy`
-- (verified on 5.22) sends single-statement migrations unwrapped, so
-- CONCURRENTLY works; multi-statement migrations are wrapped in a transaction,
-- where PostgreSQL rejects DROP INDEX CONCURRENTLY (SQLSTATE 25001,
-- https://www.postgresql.org/docs/current/sql-dropindex.html). CONCURRENTLY
-- lets CI drop this 4.8 GB index on prod without an ACCESS EXCLUSIVE lock;
-- the index is redundant with (gameServerId, createdAt).
-- DropIndex
DROP INDEX CONCURRENTLY IF EXISTS "GameServerSnapshot_createdAt_idx";
