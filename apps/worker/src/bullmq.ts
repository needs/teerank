import { RedisOptions } from "bullmq";
import { REDIS_HOST, REDIS_PORT } from "./redis";

export const bullmqConnection: RedisOptions = {
  host: REDIS_HOST,
  port: REDIS_PORT,
  family: 6,
};
