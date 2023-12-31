import { z } from "zod";

export const paramsSchema = z.object({
  clanName: z.string().transform(decodeURIComponent),
});
