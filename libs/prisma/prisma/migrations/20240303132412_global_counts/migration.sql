-- CreateTable
CREATE TABLE "GlobalCounts" (
    "globalId" TEXT NOT NULL,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "playerCount" INTEGER NOT NULL DEFAULT 0,
    "clanCount" INTEGER NOT NULL DEFAULT 0,
    "gameServerCount" INTEGER NOT NULL DEFAULT 0,

    CONSTRAINT "GlobalCounts_pkey" PRIMARY KEY ("globalId")
);
