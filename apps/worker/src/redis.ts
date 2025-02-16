import { REDIS_HOST, REDIS_PORT, REDIS_FAMILY } from '@teerank/teerank';
import { Redis } from 'ioredis';

export const redis = new Redis({
  host: REDIS_HOST,
  port: REDIS_PORT,
  family: REDIS_FAMILY,
});

redis.on('error', err => console.error('Redis Client Error', err));
