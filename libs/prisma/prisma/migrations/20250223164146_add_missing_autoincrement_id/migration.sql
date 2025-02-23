/*
  Warnings:

  - A unique constraint covering the columns `[id]` on the table `Clan` will be added. If there are existing duplicate values, this will fail.
  - A unique constraint covering the columns `[id]` on the table `GameType` will be added. If there are existing duplicate values, this will fail.
  - A unique constraint covering the columns `[id]` on the table `Player` will be added. If there are existing duplicate values, this will fail.

*/
-- AlterTable
ALTER TABLE "Clan" ADD COLUMN     "id" SERIAL NOT NULL;

-- AlterTable
ALTER TABLE "GameType" ADD COLUMN     "id" SERIAL NOT NULL;

-- AlterTable
ALTER TABLE "Player" ADD COLUMN     "id" SERIAL NOT NULL;

-- CreateIndex
CREATE UNIQUE INDEX "Clan_id_key" ON "Clan"("id");

-- CreateIndex
CREATE UNIQUE INDEX "GameType_id_key" ON "GameType"("id");

-- CreateIndex
CREATE UNIQUE INDEX "Player_id_key" ON "Player"("id");
