-- Rollup tables are partitioned by range on day so retention changes are
-- DETACH+DROP instead of delete-and-vacuum. Postgres requires the partition
-- key in the primary key, which every key below satisfies. The rollup workers
-- create future partitions ahead of time (see apps/worker/src/rollup); the
-- current and next month are created here so the first job has somewhere to
-- write.

-- CreateTable
CREATE TABLE "PlayerDay" (
    "day" DATE NOT NULL,
    "playerId" INTEGER NOT NULL,
    "playTime" INTEGER NOT NULL,

    CONSTRAINT "PlayerDay_pkey" PRIMARY KEY ("day","playerId")
) PARTITION BY RANGE ("day");

-- CreateIndex
CREATE INDEX "PlayerDay_playerId_day_idx" ON "PlayerDay"("playerId", "day");

-- CreateTable
CREATE TABLE "ServerDay" (
    "day" DATE NOT NULL,
    "gameServerId" INTEGER NOT NULL,
    "avgClients" SMALLINT NOT NULL,
    "maxClients" SMALLINT NOT NULL,

    CONSTRAINT "ServerDay_pkey" PRIMARY KEY ("gameServerId","day")
) PARTITION BY RANGE ("day");

-- CreateTable
CREATE TABLE "MapDay" (
    "day" DATE NOT NULL,
    "mapId" INTEGER NOT NULL,
    "playTime" INTEGER NOT NULL,
    "playerCount" INTEGER NOT NULL,

    CONSTRAINT "MapDay_pkey" PRIMARY KEY ("mapId","day")
) PARTITION BY RANGE ("day");

-- CreateTable
CREATE TABLE "GameTypeDay" (
    "day" DATE NOT NULL,
    "gameTypeId" INTEGER NOT NULL,
    "playTime" INTEGER NOT NULL,
    "playerCount" INTEGER NOT NULL,

    CONSTRAINT "GameTypeDay_pkey" PRIMARY KEY ("gameTypeId","day")
) PARTITION BY RANGE ("day");

-- CreateTable
CREATE TABLE "ClanDay" (
    "day" DATE NOT NULL,
    "clanId" INTEGER NOT NULL,
    "playTime" INTEGER NOT NULL,
    "playerCount" SMALLINT NOT NULL,

    CONSTRAINT "ClanDay_pkey" PRIMARY KEY ("clanId","day")
) PARTITION BY RANGE ("day");

-- CreateFunction
-- DDL cannot be a prepared statement, so partition creation lives in a
-- function the rollup workers call through typedSql.
CREATE FUNCTION create_rollup_partition(table_name text, from_day date, to_day date) RETURNS text AS $$
DECLARE
  partition_name text := table_name || '_' || to_char(from_day, 'YYYY_MM');
BEGIN
  EXECUTE format(
    'CREATE TABLE IF NOT EXISTS %I PARTITION OF %I FOR VALUES FROM (%L) TO (%L)',
    partition_name, table_name, from_day, to_day
  );
  RETURN partition_name;
END;
$$ LANGUAGE plpgsql;

-- CreatePartitions
CREATE TABLE "PlayerDay_2026_08" PARTITION OF "PlayerDay" FOR VALUES FROM ('2026-08-01') TO ('2026-09-01');
CREATE TABLE "PlayerDay_2026_09" PARTITION OF "PlayerDay" FOR VALUES FROM ('2026-09-01') TO ('2026-10-01');
CREATE TABLE "ServerDay_2026_08" PARTITION OF "ServerDay" FOR VALUES FROM ('2026-08-01') TO ('2026-09-01');
CREATE TABLE "ServerDay_2026_09" PARTITION OF "ServerDay" FOR VALUES FROM ('2026-09-01') TO ('2026-10-01');
CREATE TABLE "MapDay_2026_08" PARTITION OF "MapDay" FOR VALUES FROM ('2026-08-01') TO ('2026-09-01');
CREATE TABLE "MapDay_2026_09" PARTITION OF "MapDay" FOR VALUES FROM ('2026-09-01') TO ('2026-10-01');
CREATE TABLE "GameTypeDay_2026_08" PARTITION OF "GameTypeDay" FOR VALUES FROM ('2026-08-01') TO ('2026-09-01');
CREATE TABLE "GameTypeDay_2026_09" PARTITION OF "GameTypeDay" FOR VALUES FROM ('2026-09-01') TO ('2026-10-01');
CREATE TABLE "ClanDay_2026_08" PARTITION OF "ClanDay" FOR VALUES FROM ('2026-08-01') TO ('2026-09-01');
CREATE TABLE "ClanDay_2026_09" PARTITION OF "ClanDay" FOR VALUES FROM ('2026-09-01') TO ('2026-10-01');
