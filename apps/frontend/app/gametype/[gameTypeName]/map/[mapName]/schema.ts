import { z } from "zod";
import { paramsSchema as gameTypeNameParamsSchema, searchParamsSchema as gameTypeNameSearchParamsSchema } from "../../../../gametype/[gameTypeName]/schema";

export const paramsSchema = z.object({
  mapName: z.string().transform(decodeURIComponent),
}).merge(gameTypeNameParamsSchema);

export const searchParamsSchema = gameTypeNameSearchParamsSchema;
