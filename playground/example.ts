import { PrismaClient } from "@prisma/client";

export const prismaDatabaseUrl = process.env.DATABASE_URL;

export const prisma = new PrismaClient({
  datasourceUrl: prismaDatabaseUrl,
});

async function main() {
  // This is a playground to test the code.  Run it using `npm run playground`.
}

main()
