import { z } from "zod";
import { decodeString } from "../../../utils/encoding";

export const paramsSchema = z.object({
  playerName: z.string().transform(decodeString),
});
