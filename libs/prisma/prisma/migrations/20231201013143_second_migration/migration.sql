-- CreateEnum
CREATE TYPE "TaskRunStatus" AS ENUM ('INCOMPLETE', 'COMPLETED', 'FAILED');

-- DropForeignKey
ALTER TABLE "GameServer" DROP CONSTRAINT "GameServer_masterServerId_fkey";

-- AlterTable
ALTER TABLE "GameServer" ALTER COLUMN "masterServerId" DROP NOT NULL;

-- CreateTable
CREATE TABLE "Task" (
    "name" TEXT NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "completedAt" TIMESTAMP(3),

    CONSTRAINT "Task_pkey" PRIMARY KEY ("name")
);

-- CreateTable
CREATE TABLE "TaskRun" (
    "id" SERIAL NOT NULL,
    "taskName" TEXT NOT NULL,
    "startedAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "finishedAt" TIMESTAMP(3),
    "status" "TaskRunStatus",

    CONSTRAINT "TaskRun_pkey" PRIMARY KEY ("id")
);

-- AddForeignKey
ALTER TABLE "GameServer" ADD CONSTRAINT "GameServer_masterServerId_fkey" FOREIGN KEY ("masterServerId") REFERENCES "MasterServer"("id") ON DELETE SET NULL ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "TaskRun" ADD CONSTRAINT "TaskRun_taskName_fkey" FOREIGN KEY ("taskName") REFERENCES "Task"("name") ON DELETE RESTRICT ON UPDATE CASCADE;
