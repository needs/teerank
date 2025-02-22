/* eslint-disable */
export default {
  displayName: 'worker',
  preset: '../../jest.preset.js',
  clearMocks: true,
  testEnvironment: 'node',
  transform: {
    '^.+\\.[tj]s$': ['ts-jest', { tsconfig: '<rootDir>/tsconfig.spec.json' }],
  },
  moduleFileExtensions: ['ts', 'js', 'html'],
  coverageDirectory: '../../coverage/apps/worker',
  setupFilesAfterEnv: ['./test/mockPrisma.ts'],
};
