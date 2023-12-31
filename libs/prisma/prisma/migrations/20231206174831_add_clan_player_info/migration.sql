-- CreateTable
CREATE TABLE "ClanPlayerInfo" (
    "clanName" TEXT NOT NULL,
    "playerName" TEXT NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "playTime" INTEGER NOT NULL DEFAULT 0
);

-- CreateIndex
CREATE UNIQUE INDEX "ClanPlayerInfo_clanName_playerName_key" ON "ClanPlayerInfo"("clanName", "playerName");

-- AddForeignKey
ALTER TABLE "ClanPlayerInfo" ADD CONSTRAINT "ClanPlayerInfo_clanName_fkey" FOREIGN KEY ("clanName") REFERENCES "Clan"("name") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ClanPlayerInfo" ADD CONSTRAINT "ClanPlayerInfo_playerName_fkey" FOREIGN KEY ("playerName") REFERENCES "Player"("name") ON DELETE CASCADE ON UPDATE CASCADE;
