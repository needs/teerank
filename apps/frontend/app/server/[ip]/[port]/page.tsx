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
import { GameServer } from '@prisma/client';
import { formatDuration, intervalToDuration } from 'date-fns';
import { Metadata } from 'next';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { ip, port } = paramsSchema.parse(params);

  return {
    title: `Server ${ip}:${port}`,
    description: `List of ranked players for ${ip}:${port}`,
    alternates: {
      canonical: `https://teerank.io/server/${encodeString(ip)}/${encodeString(port.toString())}`,
    },
  };
}

function ipAndPort(ip: string, port: number) {
  switch (isIP(ip)) {
    case 6:
      return `[${ip}]:${port}`;
    default:
      return `${ip}:${port}`;
  }
}

function OfflineServer({ gameServer }: { gameServer: GameServer }) {
  return (
    <main className="flex flex-col gap-8 py-12">
      <header className="flex flex-row px-20 gap-8">
        <section className="flex flex-col gap-2 grow">
          <h1 className="text-2xl font-bold">
            {ipAndPort(gameServer.ip, gameServer.port)}
          </h1>
          <div className="flex flex-row divide-x">
            <span className="pr-4">
              Offline for{' '}
              {formatDuration(
                intervalToDuration({
                  start: gameServer.lastSeenAt,
                  end: new Date(),
                })
              )}
            </span>
          </div>
        </section>
      </header>
    </main>
  );
}

export default async function Index({
  params,
  searchParams,
}: {
  params: z.infer<typeof paramsSchema>;
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const parsedParams = paramsSchema.safeParse(params);
  const { page } = searchParamPageSchema.parse(searchParams);

  if (!parsedParams.success) {
    return notFound();
  }

  const { ip, port } = parsedParams.data;

  const gameServer = await prisma.gameServer.findUnique({
    where: {
      ip_port: {
        ip,
        port,
      },
    },
    include: {
      gameServerState: {
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

  if (gameServer === null) {
    return notFound();
  }

  if (gameServer.gameServerState === null) {
    return <OfflineServer gameServer={gameServer} />;
  }

  return (
    <main className="flex flex-col gap-8 py-12">
      <header className="flex flex-row px-20 gap-8">
        <section className="flex flex-col gap-2 grow">
          <h1 className="text-2xl font-bold">
            {gameServer.gameServerState.name}
          </h1>
          <div className="flex flex-row divide-x">
            <span className="pr-4">
              <Link
                className="hover:underline"
                href={{
                  pathname: `/gametype/${encodeString(
                    gameServer.gameServerState.map.gameTypeName
                  )}`,
                }}
              >
                {gameServer.gameServerState.map.gameTypeName}
              </Link>
            </span>
            <span className="px-4">
              <Link
                className="hover:underline"
                href={{
                  pathname: `/gametype/${encodeString(
                    gameServer.gameServerState.map.gameTypeName
                  )}/map/${encodeString(gameServer.gameServerState.map.name)}`,
                }}
              >
                {gameServer.gameServerState.map.name}
              </Link>
            </span>
            <span className="px-4">{`${gameServer.gameServerState.numClients} / ${gameServer.gameServerState.maxClients} clients`}</span>
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
        pageCount={Math.ceil(gameServer.gameServerState._count.clients / 100)}
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
        {gameServer.gameServerState.clients.map((client, index) => (
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
