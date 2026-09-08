import { PrismaClient } from "@prisma/client";

export const prismaDatabaseUrl = process.env.DATABASE_URL;

export const prisma = new PrismaClient({
  datasourceUrl: prismaDatabaseUrl,
});

function withConnectionLimit(url: string | undefined, limit: number) {
  if (url === undefined) {
    return undefined;
  }
  const parsed = new URL(url);
  parsed.searchParams.set('connection_limit', String(limit));
  return parsed.toString();
}

// The day rollup streams a whole day in small batches; on the shared pool each
// batch queues behind the hundred concurrent poll transactions and the job
// runs out its time budget while both the database and the worker sit idle.
export const rollupPrisma = new PrismaClient({
  datasourceUrl: withConnectionLimit(prismaDatabaseUrl, 2),
});
