import { List, ListCell } from '@teerank/frontend/components/List';
import prisma from '@teerank/frontend/utils/prisma';
import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import { isIP, isIPv6 } from 'net';
import Link from 'next/link';
import Image from 'next/image';

export const metadata = {
  title: 'Player',
  description: 'A Teeworlds player',
};

export default async function Index({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}) {
  const parsedParams = paramsSchema.parse(params);

  const player = await prisma.player.findUnique({
    where: {
      name: parsedParams.playerName,
    },
  });

  if (player === null) {
    return notFound();
  }

  return (
    <main className="flex flex-col gap-8 py-12">
      <header className="flex flex-row px-20 gap-4 items-center">
        <Image src="/player.png" width={100} height={100} alt="Player" />
        <section className="flex flex-col gap-2 grow">
          <h1 className="text-2xl font-bold">{player.name}</h1>
          <span className="pr-4">
            <Link
              className="hover:underline"
              href={{
                pathname: `/clan/${encodeURIComponent('Test')}`,
              }}
            >
              {'Test'}
            </Link>
          </span>
        </section>
        <aside className="flex flex-col gap-2 text-right">
          <p><b>Playtime</b>: 1045h</p>
          <p>7 hours ago</p>
        </aside>
      </header>
    </main>
  );
}
