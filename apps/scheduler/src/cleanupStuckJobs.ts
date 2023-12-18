import { subMinutes } from "date-fns";
import { prisma } from "./prisma";

export async function cleanupStuckJobs() {
  const result = await prisma.job.deleteMany({
    where: {
      startedAt: {
        lt: subMinutes(new Date(), 5),
      },
    },
  });

  if (result.count > 0) {
    console.warn(`Deleted ${result.count} stuck jobs`);
  }
}
