/*
  Warnings:

  - A unique constraint covering the columns `[jobType,rangeStart,rangeEnd]` on the table `Job` will be added. If there are existing duplicate values, this will fail.

*/
-- CreateIndex
CREATE UNIQUE INDEX "Job_jobType_rangeStart_rangeEnd_key" ON "Job"("jobType", "rangeStart", "rangeEnd");
