import { PrismaClient } from '@prisma/client'
import { mockDeep, DeepMockProxy } from 'jest-mock-extended'

import { prisma } from '../src/prisma'

jest.mock('../src/prisma', () => {
  const prisma = mockDeep<PrismaClient>()
  return { __esModule: true, prisma, rollupPrisma: prisma }
})

export const prismaMock = prisma as unknown as DeepMockProxy<PrismaClient>
