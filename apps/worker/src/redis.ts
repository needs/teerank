import { createClient } from 'redis';
import { REDIS_HOST, REDIS_PORT, REDIS_FAMILY } from '@teerank/teerank';

export const redisClientPromise = createClient({
  url: `redis://${REDIS_HOST}:${REDIS_PORT}`,
  socket: {
    family: REDIS_FAMILY,
  },
})
  .on('error', err => console.log('Redis Client Error', err))
  .connect();

export type RedisClient = Awaited<typeof redisClientPromise>;
