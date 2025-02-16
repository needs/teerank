import { Redis } from 'ioredis'

const redisClientSingleton = () => {
  return new Redis()
}

type RedisClientSingleton = ReturnType<typeof redisClientSingleton>

const globalForRedis = globalThis as unknown as {
  redis: RedisClientSingleton | undefined
}

const redis = globalForRedis.redis ?? redisClientSingleton()

export default redis

if (process.env.NODE_ENV !== 'production') globalForRedis.redis = redis
