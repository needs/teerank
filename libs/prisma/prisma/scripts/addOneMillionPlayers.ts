import { PrismaClient } from '@prisma/client'
import { faker } from '@faker-js/faker'

const prisma = new PrismaClient()

// Create players in batches to avoid memory issues
const BATCH_SIZE = 1000
const TOTAL_PLAYERS = 1_600_000

async function main() {
  console.log('Starting seed...')

  // Get current player count
  const currentCount = await prisma.player.count()
  const remainingPlayers = Math.max(0, TOTAL_PLAYERS - currentCount)

  console.log(`Current player count: ${currentCount}`)
  console.log(`Players to create: ${remainingPlayers}`)

  for (let i = 0; i < remainingPlayers; i += BATCH_SIZE) {
    const batchSize = Math.min(BATCH_SIZE, remainingPlayers - i)
    const playerData = Array.from({ length: batchSize }, () => ({
      name: faker.person.fullName(),
      createdAt: faker.date.past(),
      updatedAt: new Date(),
    }))

    await prisma.player.createMany({
      data: playerData,
      skipDuplicates: true,
    })

    console.log(`Created players ${i + 1} to ${i + batchSize}`)
  }

  console.log('Seeding finished!')
}

main()
  .catch((e) => {
    console.error(e)
    process.exit(1)
  })
  .finally(async () => {
    await prisma.$disconnect()
    process.exit(0)
  })
