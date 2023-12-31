/*
  Warnings:

  - You are about to drop the `Task` table. If the table is not empty, all the data it contains will be lost.
  - You are about to drop the `TaskRun` table. If the table is not empty, all the data it contains will be lost.

*/
-- DropForeignKey
ALTER TABLE "TaskRun" DROP CONSTRAINT "TaskRun_taskName_fkey";

-- DropTable
DROP TABLE "Task";

-- DropTable
DROP TABLE "TaskRun";

-- DropEnum
DROP TYPE "TaskRunStatus";
