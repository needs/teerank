jest.mock('ioredis', () => ({
  Redis: require('ioredis-mock')
}))
