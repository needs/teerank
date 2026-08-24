import { paramsSchema } from '../schema';
import { z } from 'zod';
import { permanentRedirect } from 'next/navigation';
import { encodeString } from '../../../../utils/encoding';

export default function Index({
  params,
  searchParams,
}: {
  params: z.infer<typeof paramsSchema>;
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { playerName } = paramsSchema.parse(params);
  const query = searchParams['show'] === 'maps' ? '?show=maps' : '';

  permanentRedirect(`/player/${encodeString(playerName)}${query}`);
}
