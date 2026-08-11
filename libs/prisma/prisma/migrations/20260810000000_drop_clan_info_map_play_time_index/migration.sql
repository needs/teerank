-- ⚠️ Must stay a single statement so CONCURRENTLY runs unwrapped — see
-- 20260810000000_drop_snapshot_created_at_index. Effectively unused (148 MB).
-- DropIndex
DROP INDEX CONCURRENTLY IF EXISTS "ClanInfoMap_playTime_idx";
