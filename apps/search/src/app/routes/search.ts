import { FastifyInstance } from 'fastify';
import { searchClans, searchPlayers } from '../../indexer';
import { z } from 'zod';

const querySchema = z.object({
  query: z.string(),
});

export default async function (fastify: FastifyInstance) {
  fastify.get('/search', function (request) {
    const { query } = querySchema.parse(request.query);
    const players = searchPlayers(query);
    const clans = searchClans(query);
    return { players, clans };
  });
}
