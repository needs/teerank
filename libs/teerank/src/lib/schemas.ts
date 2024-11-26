import { z } from 'zod';

export const indexPlayerSchema = z.object({
  name: z.string(),
  clanName: z.string().nullable(),
  playTime: z.number(),
  updatedAt: z.coerce.date(),
  lastSeenAt: z.coerce.date(),
  gameServers: z.array(z.object({
    ip: z.string(),
    port: z.number(),
  })
    .required(),
  ),
}).required();

export type IndexedPlayer = z.infer<typeof indexPlayerSchema>;

export const indexClanSchema = z.object({
  name: z.string(),
  playTime: z.number(),
  updatedAt: z.coerce.date(),
  playerCount: z.number(),
}).required();

export type IndexedClan = z.infer<typeof indexClanSchema>;
