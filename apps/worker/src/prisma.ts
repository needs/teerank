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

export const rollupPrisma = new PrismaClient({
  datasourceUrl: withConnectionLimit(prismaDatabaseUrl, 2),
});
