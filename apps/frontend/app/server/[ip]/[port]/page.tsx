import { List, ListCell } from '@teerank/frontend/components/List';
import prisma from '@teerank/frontend/utils/prisma';
import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import { isIP, isIPv6 } from 'net';
import Link from 'next/link';

export const metadata = {
  title: 'Server',
  description: 'A teerank server',
};

function ipAndPort(ip: string, port: number) {
  switch (isIP(ip)) {
    case 6:
      return `[${ip}]:${port}`;
    default:
      return `${ip}:${port}`;
  }
}

export default async function Index({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}) {
  const parsedParams = paramsSchema.parse(params);

  const snapshot = await prisma.gameServerSnapshot.findFirst({
    where: {
      gameServer: {
        ip: parsedParams.ip,
        port: parsedParams.port,
      },
    },
    orderBy: {
      createdAt: 'desc',
    },
    include: {
      clients: true,
      gameServer: true,
      map: true,
    },
  });

  if (!snapshot) {
    return notFound();
  }

  return (
    <main className="flex flex-col gap-8 py-12">
      <header className="flex flex-row justify-between px-20">
        <section className="flex flex-col gap-2">
          <h1 className="text-2xl font-bold">{snapshot.name}</h1>
          <div className="flex flex-row divide-x">
            <span className="pr-4">
              <Link
                className="hover:underline"
                href={{
                  pathname: `/gametype/${encodeURIComponent(
                    snapshot.map.gameTypeName
                  )}`,
                }}
              >
                {snapshot.map.gameTypeName}
              </Link>
            </span>
            <span className="px-4">
              <Link
                className="hover:underline"
                href={{
                  pathname: `/gametype/${encodeURIComponent(
                    snapshot.map.gameTypeName
                  )}`,
                  query: {
                    map: snapshot.map.name,
                  },
                }}
              >
                {snapshot.map.name}
              </Link>
            </span>
            <span className="px-4">{`${snapshot.numClients} / ${snapshot.maxClients} clients`}</span>
            <span className="px-4">{`${snapshot.numPlayers} players`}</span>
          </div>
        </section>
        <section className="flex flex-col divide-y border rounded-md justify-stretch items-stretch overflow-hidden">
          <h2 className="py-1.5 px-4 bg-gradient-to-b from-[#ffffff] to-[#eaeaea] text-center">
            Server address
          </h2>
          <span className="py-1.5 px-4 text-sm">
            {ipAndPort(snapshot.gameServer.ip, snapshot.gameServer.port)}
          </span>
        </section>
      </header>

      <List
        columns={[
          {
            title: '',
            expand: false,
          },
          {
            title: 'Name',
            expand: true,
          },
          {
            title: 'Clan',
            expand: true,
          },
          {
            title: 'Score',
            expand: false,
          },
        ]}
      >
        {snapshot.clients.map((client, index) => (
          <>
            <ListCell alignRight label={`${index + 1}`} />
            <ListCell
              label={client.playerName}
              href={{ pathname: `/player/${client.playerName}` }}
            />
            <ListCell label={client.clan} />
            <ListCell alignRight label={client.score.toString()} />
          </>
        ))}
      </List>
    </main>
  );
}
