/*
  Warnings:

  - You are about to alter the column `country` on the `GameServerClient` table. The data in that column could be lost. The data in that column will be cast from `VarChar(191)` to `Int`.

*/
-- AlterTable
ALTER TABLE `GameServerClient` MODIFY `country` INTEGER NOT NULL;
