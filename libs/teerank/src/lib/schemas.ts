import { z } from 'zod';

export const indexPlayerSchema = z.object({
  name: z.string(),
  clanName: z.string().nullable(),
  playTime: z.number(),
});

export type IndexedPlayer = z.infer<typeof indexPlayerSchema>;

export const indexClanSchema = z.object({
  name: z.string(),
  playTime: z.number(),
});

export type IndexedClan = z.infer<typeof indexClanSchema>;
