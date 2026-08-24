ALTER TABLE "Player"
  ADD COLUMN "pollCount" INTEGER NOT NULL DEFAULT 0,
  ADD COLUMN "occurrenceCount" INTEGER NOT NULL DEFAULT 0;

-- Not partitioned: rows are upserted in place, not appended per day.
CREATE TABLE "PlayerPartner" (
    "playerId" INTEGER NOT NULL,
    "partnerId" INTEGER NOT NULL,
    "playTime" INTEGER NOT NULL,
    "lastPlayedAt" DATE NOT NULL,

    CONSTRAINT "PlayerPartner_pkey" PRIMARY KEY ("playerId", "partnerId")
);
