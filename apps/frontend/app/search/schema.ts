import { z } from "zod";

export const searchParamSchema = z
  .object({
    query: z.string().optional().default(""),
  })
  .catch({ query: "" });
