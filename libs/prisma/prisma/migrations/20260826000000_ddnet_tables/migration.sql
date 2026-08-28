-- DDNet official stats tables. Same conventions as the rollup tables: no
-- foreign keys, ids resolved at write time. finishCount on the partitioned
-- rollup tables is added with a constant default, which is metadata-only.

-- AlterTable
ALTER TABLE "PlayerDay" ADD COLUMN "finishCount" INTEGER NOT NULL DEFAULT 0;

-- AlterTable
ALTER TABLE "MapDay" ADD COLUMN "finishCount" INTEGER NOT NULL DEFAULT 0;

-- CreateTable
CREATE TABLE "DdnetMap" (
    "mapId" INTEGER NOT NULL,
    "category" TEXT NOT NULL,
    "points" SMALLINT NOT NULL,
    "stars" SMALLINT NOT NULL,
    "mapper" TEXT NOT NULL,
    "releasedAt" DATE,
    "width" INTEGER,
    "height" INTEGER,
    "tiles" TEXT[],
    "finishCount" INTEGER NOT NULL DEFAULT 0,
    "medianTime" DOUBLE PRECISION,
    "wrSplits" REAL[],
    "medianSplits" REAL[],
    "timeQuantiles" REAL[],

    CONSTRAINT "DdnetMap_pkey" PRIMARY KEY ("mapId")
);

-- CreateTable
CREATE TABLE "DdnetMapMapper" (
    "mapperName" TEXT NOT NULL,
    "mapId" INTEGER NOT NULL,

    CONSTRAINT "DdnetMapMapper_pkey" PRIMARY KEY ("mapperName","mapId")
);

-- CreateIndex
CREATE INDEX "DdnetMapMapper_mapId_idx" ON "DdnetMapMapper"("mapId");

-- CreateTable
CREATE TABLE "DdnetMapThumb" (
    "mapId" INTEGER NOT NULL,
    "png" BYTEA NOT NULL,
    "etag" TEXT,
    "fetchedAt" TIMESTAMP(3) NOT NULL,

    CONSTRAINT "DdnetMapThumb_pkey" PRIMARY KEY ("mapId")
);

-- CreateTable
CREATE TABLE "DdnetRaceBest" (
    "mapId" INTEGER NOT NULL,
    "playerId" INTEGER NOT NULL,
    "time" DOUBLE PRECISION NOT NULL,
    "finishCount" INTEGER NOT NULL,
    "firstFinishAt" DATE NOT NULL,
    "bestAt" DATE NOT NULL,
    "splits" REAL[],

    CONSTRAINT "DdnetRaceBest_pkey" PRIMARY KEY ("mapId","playerId")
);

-- CreateIndex
CREATE INDEX "DdnetRaceBest_mapId_time_idx" ON "DdnetRaceBest"("mapId", "time");

-- CreateIndex
CREATE INDEX "DdnetRaceBest_playerId_idx" ON "DdnetRaceBest"("playerId");

-- CreateTable
CREATE TABLE "DdnetPlayer" (
    "playerId" INTEGER NOT NULL,
    "points" INTEGER NOT NULL DEFAULT 0,
    "pointsRank" INTEGER,
    "finishCount" INTEGER NOT NULL DEFAULT 0,
    "firstFinishAt" TIMESTAMP(3) NOT NULL,
    "lastFinishAt" TIMESTAMP(3) NOT NULL,

    CONSTRAINT "DdnetPlayer_pkey" PRIMARY KEY ("playerId")
);

-- CreateIndex
CREATE INDEX "DdnetPlayer_points_idx" ON "DdnetPlayer"("points" DESC);

-- CreateTable
CREATE TABLE "DdnetTeamRace" (
    "teamId" BYTEA NOT NULL,
    "mapId" INTEGER NOT NULL,
    "time" DOUBLE PRECISION NOT NULL,
    "timestamp" TIMESTAMP(3) NOT NULL,
    "size" SMALLINT NOT NULL,

    CONSTRAINT "DdnetTeamRace_pkey" PRIMARY KEY ("teamId")
);

-- CreateIndex
CREATE INDEX "DdnetTeamRace_mapId_time_idx" ON "DdnetTeamRace"("mapId", "time");

-- CreateTable
CREATE TABLE "DdnetTeamRacePlayer" (
    "teamId" BYTEA NOT NULL,
    "playerId" INTEGER NOT NULL,

    CONSTRAINT "DdnetTeamRacePlayer_pkey" PRIMARY KEY ("teamId","playerId")
);

-- CreateIndex
CREATE INDEX "DdnetTeamRacePlayer_playerId_idx" ON "DdnetTeamRacePlayer"("playerId");

-- CreateTable
CREATE TABLE "DdnetPartner" (
    "playerId" INTEGER NOT NULL,
    "partnerId" INTEGER NOT NULL,
    "finishCount" INTEGER NOT NULL,
    "bestTime" DOUBLE PRECISION NOT NULL,
    "bestMapId" INTEGER NOT NULL,
    "lastFinishAt" DATE NOT NULL,

    CONSTRAINT "DdnetPartner_pkey" PRIMARY KEY ("playerId","partnerId")
);

-- CreateTable
CREATE TABLE "DdnetPartnerMap" (
    "playerId" INTEGER NOT NULL,
    "partnerId" INTEGER NOT NULL,
    "mapId" INTEGER NOT NULL,
    "bestTime" DOUBLE PRECISION NOT NULL,
    "finishCount" INTEGER NOT NULL,
    "lastFinishAt" DATE NOT NULL,

    CONSTRAINT "DdnetPartnerMap_pkey" PRIMARY KEY ("playerId","partnerId","mapId")
);

-- CreateTable
CREATE TABLE "DdnetRecord" (
    "mapId" INTEGER NOT NULL,
    "timestamp" TIMESTAMP(3) NOT NULL,
    "playerId" INTEGER NOT NULL,
    "time" DOUBLE PRECISION NOT NULL,

    CONSTRAINT "DdnetRecord_pkey" PRIMARY KEY ("mapId","timestamp","playerId")
);

-- CreateIndex
CREATE INDEX "DdnetRecord_timestamp_idx" ON "DdnetRecord"("timestamp");

-- CreateTable
CREATE TABLE "DdnetOnlineDay" (
    "day" DATE NOT NULL,
    "kind" SMALLINT NOT NULL,
    "key" TEXT NOT NULL,
    "avgPlayers" INTEGER NOT NULL,
    "maxPlayers" INTEGER NOT NULL,

    CONSTRAINT "DdnetOnlineDay_pkey" PRIMARY KEY ("day","kind","key")
);

-- CreateTable
CREATE TABLE "DdnetState" (
    "key" TEXT NOT NULL,
    "value" JSONB NOT NULL,

    CONSTRAINT "DdnetState_pkey" PRIMARY KEY ("key")
);
