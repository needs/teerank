import { indexClanSchema, indexPlayerSchema } from "@teerank/teerank";
import { z } from "zod";

const SEARCH_API_URL = process.env.SEARCH_API_URL ?? 'http://localhost:3001';

const searchDataSchema = z.object({
  players: z.array(indexPlayerSchema),
  clans: z.array(indexClanSchema),
});

export async function search(query: string) {
  return await fetch(`${SEARCH_API_URL}/search?query=${query || "name"}`)
    .then((res) => res.json())
    .then((data) => searchDataSchema.parse(data));
}
