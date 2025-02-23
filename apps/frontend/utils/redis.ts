import { REDIS_FAMILY, REDIS_HOST, REDIS_PORT } from '@teerank/teerank'
import { Redis } from 'ioredis'

const redisClientSingleton = () => {
  return new Redis({
    host: REDIS_HOST,
    port: REDIS_PORT,
    family: REDIS_FAMILY,
  })
}

type RedisClientSingleton = ReturnType<typeof redisClientSingleton>

const globalForRedis = globalThis as unknown as {
  redis: RedisClientSingleton | undefined
}

const redis = globalForRedis.redis ?? redisClientSingleton()

export default redis

if (process.env.NODE_ENV !== 'production') globalForRedis.redis = redis
