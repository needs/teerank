import { PrismaClient } from "@prisma/client";

export const prismaDatabaseUrl = process.env.DATABASE_URL;

export const prisma = new PrismaClient({
  datasourceUrl: prismaDatabaseUrl,
});
