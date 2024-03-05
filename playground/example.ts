import { PrismaClient } from "@prisma/client";

export const prismaDatabaseUrl = process.env.DATABASE_URL;

export const prisma = new PrismaClient({
  datasourceUrl: prismaDatabaseUrl,
});

async function main() {
  // This is a playground to test the code.  Run it using `npm run playground:dev`
  // or `npm run playground:prod`.  Be careful with the code you write here if
  // you are running it against prod.
}

main()
