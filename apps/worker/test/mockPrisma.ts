import { PrismaClient } from '@prisma/client'
import { mockDeep, DeepMockProxy } from 'jest-mock-extended'

import { prisma } from '../src/prisma'

jest.mock('../src/prisma', () => ({
  __esModule: true,
  prisma: mockDeep<PrismaClient>(),
}))

export const prismaMock = prisma as unknown as DeepMockProxy<PrismaClient>
