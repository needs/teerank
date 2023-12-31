import { z } from "zod";

export const searchParamSchema = z
  .object({
    page: z.coerce.number().optional().default(1),
  })
  .catch({ page: 1 });
