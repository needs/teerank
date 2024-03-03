import { z } from "zod";
import { paramsSchema as gameTypeNameParamsSchema, searchParamsSchema as gameTypeNameSearchParamsSchema } from "../../../../gametype/[gameTypeName]/schema";
import { decodeString } from "../../../../../utils/encoding";

export const paramsSchema = z.object({
  mapName: z.string().transform(decodeString),
}).merge(gameTypeNameParamsSchema);

export const searchParamsSchema = gameTypeNameSearchParamsSchema;
