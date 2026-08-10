# Cost reduction — what merging this branch does

Everything is code-driven: merging to master applies it all through the normal CI pipeline.
The disk situation is not an emergency — ~3.4 GB of headroom at ~103 MB/day growth leaves a
few weeks of margin, and the changes below both add headroom and remove the growth source.

## Applied automatically on merge

**Migrations** (CI runs `prisma migrate deploy` before deploying the apps):

- Three single-statement `DROP INDEX CONCURRENTLY` migrations reclaim ~5.8 GB immediately
  and lock-free: `GameServerSnapshot_createdAt_idx` (4.8 GB, redundant with
  `(gameServerId, createdAt)`), `PlayerInfoMap_playTime_idx` (805 MB, unused), and
  `ClanInfoMap_playTime_idx` (148 MB). They must stay single-statement — Prisma wraps
  multi-statement migrations in a transaction, where PostgreSQL rejects CONCURRENTLY
  (verified on Prisma 5.22; see the comments in the migration files). If a CI run is
  cancelled mid-drop the index can be left `INVALID` but present; the next run re-drops it.
- `GameServerState.gameServerId` becomes NOT NULL (orphans deleted first). Old workers
  briefly fail polls between the migrate step and the worker deploy — harmless,
  self-healing.
- `GameServerStateClient.id` widens to bigint; `GameServer.masterServerId` gets an index.
- `GameServerState`/`GameServerStateClient` get `fillfactor = 70` so the new in-place
  state updates stay HOT, and the two snapshot tables get eager autovacuum settings so
  the archive drain's deletes are reclaimed as fast as they're made.

**Deploys**: the new write path (state upsert, batched typedSql writes, one transaction per
poll) and the archive worker ship with the worker/scheduler images. CI then pins the worker
fleet to one machine (`flyctl scale count 1`) — the measured workload needs ~6 concurrent
poll slots and one machine provides 100.

**Archiving** starts by itself: the scheduler ticks `archive-snapshots` every 10 minutes.
In production the worker stays idle (with a log line) until `S3_ENDPOINT` is set, so the
deploy is safe before the R2 bucket exists. Once configured, it drains the ~180 M-row
backlog within its time budget per tick and settles into steady state on its own; nothing
special-cases the backlog.

## The one thing code can't do: R2 credentials

Creating the Cloudflare R2 bucket and handing the worker its credentials is inherently
out-of-repo. When ready (no urgency — archiving just waits):

```sh
fly secrets set -a teerankio-worker \
  S3_ENDPOINT='https://<account-id>.r2.cloudflarestorage.com' \
  S3_REGION='auto' \
  S3_BUCKET='teerank-snapshots' \
  S3_FORCE_PATH_STYLE='false' \
  S3_ACCESS_KEY_ID='…' \
  S3_SECRET_ACCESS_KEY='…'
```

## Tuning knobs (env on `teerankio-worker`)

| Var | Default | Meaning |
|---|---|---|
| `SNAPSHOT_RETENTION_HOURS` | `48` | rows younger than this are never archived |
| `ARCHIVE_BATCH_SIZE` | `5000` | snapshots per Parquet object |
| `ARCHIVE_TIME_BUDGET_MS` | `300000` | wall-clock budget per 10-min tick |
| `ARCHIVE_BATCH_PAUSE_MS` | `200` | sleep between batches |

The defaults drain the backlog over roughly a couple of weeks at a gentle duty cycle.
Raise the budget / lower the pause to go faster once it's proven stable; raise the pause
if `n_dead_tup` on the snapshot tables outruns autovacuum.

## What to expect and watch

- `disk-capacity` (`fly checks list -a teerankio-postgres2`) drops from ~87% to ~81–82%
  when the index-drop migrations land, then stops growing as the drain catches up.
- During the drain, occasionally glance at volume free space, `pg_wal` size,
  `n_dead_tup`/`last_autovacuum` on the two snapshot tables, and poll-queue depth (the
  `queuesFull` guard trips at 50 000).
- Post-drain: the snapshot tables hold only the 48 h window (~520 k snapshots) and
  `pg_database_size` sits near 18 GB. The *volume* still reads ~81% — Postgres doesn't
  return heap space to the OS; actually shrinking the volume is the Phase 3 dump/restore
  onto smaller hosting, which is also when the remaining index bloat (e.g. the unused
  6.2 GB `GameServerClient_pkey`) disappears for free.
- Archive completeness check: object id-ranges should tile with no gaps up to the oldest
  surviving snapshot id.

## Deliberately not done (was in the original plan)

- **Dropping `GameServerClient_pkey` (6.2 GB)**: manual-only by nature (it would drift from
  `schema.prisma`, which requires an id). With the growth source removed and ~5.8 GB
  reclaimed by the index drops, the headroom math no longer needs it; the space comes back
  at the Phase 3 dump/restore anyway.
- **`VACUUM FULL` / `REINDEX` on the state tables**: they're under 5 MB — autovacuum plus
  the new HOT-friendly write path reclaims them without exclusive locks.
- **Postgres 15.8 → 15.15**: worth doing someday via Fly's image update, unrelated to this
  branch.

## Testing locally

```sh
docker compose up -d          # includes MinIO on :9000, console on :9001 (minioadmin/minioadmin)
npx nx run worker:serve
npx nx run scheduler:serve
```

Set `SNAPSHOT_RETENTION_HOURS=0` on the worker to archive without waiting. Objects appear
in the MinIO console; rows disappear from Postgres only after a verified upload. Query the
archive straight from DuckDB:

```sql
CREATE SECRET (TYPE S3, KEY_ID 'minioadmin', SECRET 'minioadmin',
               ENDPOINT 'localhost:9000', URL_STYLE 'path', USE_SSL false);
SELECT dt, "playerName", count(*) * 5 AS approx_minutes
FROM read_parquet('s3://teerank-snapshots/snapshots/dt=*/*.parquet', hive_partitioning = 1)
WHERE "inGame" GROUP BY dt, "playerName";
```
