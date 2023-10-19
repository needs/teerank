import { PrismaClient } from '@prisma/client'

const prisma = new PrismaClient()

async function main() {
  await prisma.masterServer.createMany({
    data: [
      {
        address: 'master1.teeworlds.com',
        port: 8300,
      },
      {
        address: 'master2.teeworlds.com',
        port: 8300,
      },
      {
        address: 'master3.teeworlds.com',
        port: 8300,
      },
      {
        address: 'master4.teeworlds.com',
        port: 8300,
      },
    ],
    skipDuplicates: true,
  })
}

main()
  .then(async () => {
    await prisma.$disconnect()
  })
  .catch(async (e) => {
    console.error(e)
    await prisma.$disconnect()
    process.exit(1)
  })
