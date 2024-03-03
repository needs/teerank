import { z } from "zod";
import { decodeString } from "../../../utils/encoding";

export const searchParamsSchema = z
  .object({
    page: z.coerce.number().optional().default(1),
  })
  .catch({ page: 1 });

export const paramsSchema = z.object({
  gameTypeName: z.string().transform(decodeString),
});
