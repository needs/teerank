import { z } from "zod";
import { decodeIp } from "../../../../utils/encoding";

export const paramsSchema = z.object({
  ip: z.string().transform(decodeIp),
  port: z.coerce.number().int().positive(),
});
