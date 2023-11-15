import { z } from "zod";

export const searchParamsSchema = z
  .object({
    map: z.string().optional(),
    page: z.coerce.number().optional().default(1),
  })
  .catch({ page: 1 });

export const paramsSchema = z.object({
  gameType: z.string().transform(decodeURIComponent),
});
