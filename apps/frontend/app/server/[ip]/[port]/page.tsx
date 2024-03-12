import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import { isIP } from 'net';
import Link from 'next/link';
import { List, ListCell } from '../../../../components/List';
import { searchParamPageSchema } from '../../../../utils/page';
import prisma from '../../../../utils/prisma';
import { encodeString } from '../../../../utils/encoding';
import { formatPlayTime } from '../../../../utils/format';

export const metadata = {
  title: 'Server',
  description: 'A Teeworlds server',
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
  searchParams,
}: {
  params: z.infer<typeof paramsSchema>;
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const parsedParams = paramsSchema.parse(params);
  const { page } = searchParamPageSchema.parse(searchParams);

  const gameServer = await prisma.gameServer.findUnique({
    where: {
      ip_port: {
        ip: parsedParams.ip,
        port: parsedParams.port,
      },
    },
    include: {
      lastSnapshot: {
        include: {
          clients: {
            orderBy: {
              score: 'desc',
            },
            skip: (page - 1) * 100,
            take: 100,
          },
          _count: {
            select: {
              clients: true,
            },
          },
          map: true,
        },
      },
    },
  });

  if (gameServer === null || gameServer.lastSnapshot === null) {
    return notFound();
  }

  return (
    <main className="flex flex-col gap-8 py-12">
      <header className="flex flex-row px-20 gap-8">
        <section className="flex flex-col gap-2 grow">
          <h1 className="text-2xl font-bold">{gameServer.lastSnapshot.name}</h1>
          <div className="flex flex-row divide-x">
            <span className="pr-4">
              <Link
                className="hover:underline"
                href={{
                  pathname: `/gametype/${encodeString(
                    gameServer.lastSnapshot.map.gameTypeName
                  )}`,
                }}
              >
                {gameServer.lastSnapshot.map.gameTypeName}
              </Link>
            </span>
            <span className="px-4">
              <Link
                className="hover:underline"
                href={{
                  pathname: `/gametype/${encodeString(
                    gameServer.lastSnapshot.map.gameTypeName
                  )}/map/${encodeString(gameServer.lastSnapshot.map.name)}`,
                }}
              >
                {gameServer.lastSnapshot.map.name}
              </Link>
            </span>
            <span className="px-4">{`${gameServer.lastSnapshot.numClients} / ${gameServer.lastSnapshot.maxClients} clients`}</span>
            <span className="px-4">
              Playtime: {formatPlayTime(gameServer.playTime)}
            </span>
          </div>
        </section>
        <section className="flex flex-col divide-y border rounded-md overflow-hidden self-start">
          <h2 className="py-1.5 px-4 bg-gradient-to-b from-[#ffffff] to-[#eaeaea] text-center">
            Server address
          </h2>
          <span className="py-1.5 px-4 text-sm">
            {ipAndPort(gameServer.ip, gameServer.port)}
          </span>
        </section>
      </header>

      <List
        pageCount={Math.ceil(gameServer.lastSnapshot._count.clients / 100)}
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
        {gameServer.lastSnapshot.clients.map((client, index) => (
          <>
            <ListCell alignRight label={`${index + 1}`} />
            <ListCell
              label={client.playerName}
              href={{
                pathname: `/player/${encodeString(client.playerName)}`,
              }}
            />
            <ListCell
              label={client.clanName ?? ''}
              href={
                client.clanName === null
                  ? undefined
                  : {
                      pathname: `/clan/${encodeString(client.clanName)}`,
                    }
              }
            />
            <ListCell alignRight label={client.score.toString()} />
          </>
        ))}
      </List>
    </main>
  );
}
