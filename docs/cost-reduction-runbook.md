# Cost reduction runbook — manual production steps

The code side of the cost-reduction plan lives in this branch. This file lists every step
that has to be run **manually against production** (via `npm run proxy:database` + psql, or
flyctl), in order. Connection credentials: `fly ssh console -a teerankio-postgres2 -C printenv`
— `OPERATOR_PASSWORD` connects as `postgres` to the `teerank` database.

## Phase 0 — buy headroom (run first, before anything else)

The volume is at ~87% and Fly Postgres goes read-only at 90%. These return space to the
filesystem immediately:

```sql
DROP INDEX CONCURRENTLY "GameServerSnapshot_createdAt_idx";  -- 4.8 GB, redundant with (gameServerId, createdAt)
DROP INDEX CONCURRENTLY "PlayerInfoMap_playTime_idx";        -- 805 MB, unused
DROP INDEX CONCURRENTLY "ClanInfoMap_playTime_idx";          -- 148 MB, effectively unused
ALTER TABLE "GameServerClient" DROP CONSTRAINT "GameServerClient_pkey";  -- 6.2 GB, never scanned, no FK references it; brief ACCESS EXCLUSIVE
```

Then mark the matching migration as applied so CI doesn't re-run it:

```sh
npx prisma migrate resolve --applied 20260810000000_drop_redundant_indices --schema libs/prisma/prisma/schema.prisma
```

⚠️ The `GameServerClient_pkey` drop is deliberately **not** in the Prisma schema or any
migration — it is an operational action creating documented drift until the table is dropped
entirely. Prisma keeps working (inserts still get the sequence default).

Also scale the worker fleet down — 300 concurrent poll slots for a ~6-slot workload:

```sh
fly scale count 1 -a teerankio-worker
```

Verify: `fly checks list -a teerankio-postgres2` — `disk-capacity` should fall from ~87% to ~76%.
Confirm search and the map/clan pages still work.

## Baseline (before deploying the Phase 2 write-path changes)

With `pg_stat_statements` (already installed), capture top queries by total time and by call
count, plus `n_tup_hot_upd` ratios on `GameServerState` / `Player` / `PlayerInfoMap`.

## Phase 2a — after the state-upsert code deploys

```sql
ALTER TABLE "GameServerState"       SET (fillfactor = 70);
ALTER TABLE "GameServerStateClient" SET (fillfactor = 70);
VACUUM FULL "GameServerState";                         -- 352 KB heap, instant
VACUUM FULL "GameServerStateClient";
REINDEX INDEX CONCURRENTLY "GameServerState_name_idx"; -- reclaim GIN bloat
```

## Phase 2g — before applying the bigint migration

Check sequence positions (int4 overflow risk):

```sql
SELECT sequencename, last_value FROM pg_sequences
WHERE sequencename IN ('GameServerStateClient_id_seq', 'GameServerClient_id_seq', 'GameServerSnapshot_id_seq');
```

## Phase 1d — before ramping the archive drain

Tune autovacuum on both tables so dead tuples don't eat the Phase 0 headroom:

```sql
ALTER TABLE "GameServerClient"   SET (autovacuum_vacuum_scale_factor = 0.01,
                                      autovacuum_vacuum_cost_limit   = 2000);
ALTER TABLE "GameServerSnapshot" SET (autovacuum_vacuum_scale_factor = 0.01,
                                      autovacuum_vacuum_cost_limit   = 2000);
```

Archive worker env (set on `teerankio-worker` / `teerankio-scheduler`):

| Var | Default | Meaning |
|---|---|---|
| `S3_ENDPOINT` | — | R2 account endpoint (`https://<account>.r2.cloudflarestorage.com`) |
| `S3_REGION` | `auto` | `auto` for R2 |
| `S3_BUCKET` | `teerank-snapshots` | bucket name |
| `S3_ACCESS_KEY_ID` / `S3_SECRET_ACCESS_KEY` | — | credentials |
| `S3_FORCE_PATH_STYLE` | `false` | `true` for MinIO |
| `SNAPSHOT_RETENTION_HOURS` | `48` | rows younger than this are never archived |
| `ARCHIVE_BATCH_SIZE` | `5000` | snapshots per Parquet object |
| `ARCHIVE_TIME_BUDGET_MS` | `300000` | wall-clock budget per 10-min tick |
| `ARCHIVE_BATCH_PAUSE_MS` | `200` | sleep between batches |

Start with the conservative defaults, watch one tick, then ramp the drain by raising
`ARCHIVE_TIME_BUDGET_MS` and lowering `ARCHIVE_BATCH_PAUSE_MS`.

**Watch daily during the drain:** volume free space (`disk-capacity` check), `pg_wal` size,
`n_dead_tup` / `last_autovacuum` on both tables, and poll-queue depth (the `queuesFull` guard
trips at 50 000). If dead tuples outpace autovacuum, raise `ARCHIVE_BATCH_PAUSE_MS`.

**Post-drain:** the two tables should hold only the 48h window (~520 k snapshots);
`pg_database_size` should sit near 18 GB and stop growing. The *volume* still reads ~76%
until a dump/restore — expected, not a failure. Verify archive completeness: object id-ranges
must tile continuously with no gaps up to the oldest surviving snapshot.

## Local end-to-end test of the archive path

```sh
docker compose up -d          # now includes MinIO on :9000, console on :9001
npx nx run worker:serve       # in one shell
npx nx run scheduler:serve    # in another
```

Force `SNAPSHOT_RETENTION_HOURS=0` on the worker to exercise the path without waiting.
Confirm objects appear in the MinIO console (`localhost:9001`, minioadmin/minioadmin) and rows
then disappear from Postgres. Point DuckDB at the local bucket and diff row counts and sampled
rows against what was deleted:

```sql
CREATE SECRET (TYPE S3, KEY_ID 'minioadmin', SECRET 'minioadmin',
               ENDPOINT 'localhost:9000', URL_STYLE 'path', USE_SSL false);
SELECT dt, "playerName", count(*) * 5 AS approx_minutes
FROM read_parquet('s3://teerank-snapshots/snapshots/dt=*/*.parquet', hive_partitioning = 1)
WHERE "inGame" GROUP BY dt, "playerName";
```

Check `createdAt` returns as a real timestamp, `snapshotId` keeps precision, and nulls stay
null rather than `""`.

## Notes

- Postgres is on 15.8 with 15.15 available. Update on its own schedule — not while the volume
  is tight or the drain is running.
- Actually shrinking the 110 GB volume needs a dump/restore onto a smaller one — deferred to
  the Phase 3 hosting move, once the DB is ~18 GB.
