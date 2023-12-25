process.env.DATABASE_URL = 'postgres://postgres-test:postgres-test@localhost:5433/teerank-test';

import { prisma, prismaDatabaseUrl } from "./src/prisma";

export async function clearDatabase() {
  if (prismaDatabaseUrl !== 'postgres://postgres-test:postgres-test@localhost:5433/teerank-test') {
    throw new Error('Database URL must be set to postgres://postgres-test:postgres-test@localhost:5433/teerank-test');
  }

  for (const [modelName, model] of Object.entries(prisma)) {
    if (!modelName.startsWith('$') && !modelName.startsWith('_')) {
      // eslint-disable-next-line
      await model.deleteMany();
    }
  };
}

export async function runJobNTimes(n: number, job: () => Promise<boolean>) {
  for (let i = 0; i < n - 1; i++) {
    expect(await job()).toBe(true);
  }

  expect(await job()).toBe(false);
}
