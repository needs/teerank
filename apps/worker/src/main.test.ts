import { clearDatabase } from "../testSetup";
import { prisma } from "./prisma";

beforeEach(async () => {
  await clearDatabase();
});

test('concurreny', async () => {
  const job = await prisma.job.create({
    data: {
      jobType: 'POLL_MASTER_SERVERS',
      rangeStart: 0,
      rangeEnd: 100,
      priority: 0,
    }
  });

  const deleteJob = async () =>
    await prisma.job.delete({
      where: {
        id: job.id,
      }
    });

  await expect(deleteJob()).resolves.toEqual(job);
  await expect(deleteJob()).rejects.toThrow();
});
