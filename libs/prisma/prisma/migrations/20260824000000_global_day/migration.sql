-- CreateTable
CREATE TABLE "GlobalDay" (
    "day" DATE NOT NULL,
    "playerCount" INTEGER NOT NULL,

    CONSTRAINT "GlobalDay_pkey" PRIMARY KEY ("day")
);

-- Backfill the days already rolled up.
INSERT INTO "GlobalDay" ("day", "playerCount")
SELECT "day", count(*)::int FROM "PlayerDay" GROUP BY "day";
