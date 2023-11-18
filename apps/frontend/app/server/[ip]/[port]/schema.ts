import { z } from "zod";

export const paramsSchema = z.object({
  ip: z.string().ip(),
  port: z.coerce.number().int().positive(),
});
