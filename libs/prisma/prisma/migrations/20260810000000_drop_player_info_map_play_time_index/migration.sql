-- ⚠️ Must stay a single statement so CONCURRENTLY runs unwrapped — see
-- 20260810000000_drop_snapshot_created_at_index. Unused index (805 MB).
-- DropIndex
DROP INDEX CONCURRENTLY IF EXISTS "PlayerInfoMap_playTime_idx";
