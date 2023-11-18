import { z } from "zod";

export const paramsSchema = z.object({
  playerName: z.string().transform(decodeURIComponent),
});
