import { z } from "zod";
import { decodeString } from "../../../utils/encoding";

export const paramsSchema = z.object({
  mapperName: z.string().transform(decodeString),
});
