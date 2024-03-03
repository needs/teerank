import { z } from "zod";
import { decodeString } from "../../../utils/encoding";

export const paramsSchema = z.object({
  clanName: z.string().transform(decodeString),
});
