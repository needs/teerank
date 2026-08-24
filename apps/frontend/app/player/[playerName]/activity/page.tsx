import { paramsSchema } from '../schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import { Fragment } from 'react';
import { Metadata } from 'next';
import prisma from '../../../../utils/prisma';
import { List, ListCell } from '../../../../components/List';
import { LastSeen } from '../../../../components/LastSeen';
import { encodeIp, encodeString } from '../../../../utils/encoding';
import { formatPlayTime } from '../../../../utils/format';

// One snapshot per poll cycle, same 5 minutes the rollups count.
const OBSERVATION_SECONDS = 5 * 60;
const RECENT_CLIENT_COUNT = 500;

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { playerName } = paramsSchema.parse(params);

  return {
    title: `Player ${playerName} - Activity`,
    description: 'Servers a Teeworlds player was recently seen on',
    alternates: {
      canonical: `https://teerank.io/player/${encodeString(playerName)}/activity`,
    },
  };
}

export default async function Index({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}) {
  const { playerName } = paramsSchema.parse(params);

  const player = await prisma.player.findUnique({
    select: {
      name: true,

      gameServerStateClients: {
        select: {
          gameServerState: {
            select: {
              gameServer: {
                select: {
                  ip: true,
                  port: true,
                },
              },
            },
          },
        },
      },
    },
    where: { name: playerName },
  });

  if (player === null) {
    return notFound();
  }

  const clients = await prisma.gameServerClient.findMany({
    where: { playerName },
    orderBy: { id: 'desc' },
    take: RECENT_CLIENT_COUNT,
    select: {
      inGame: true,
      snapshot: {
        select: {
          createdAt: true,
          name: true,
          map: {
            select: {
              name: true,
              gameTypeName: true,
            },
          },
          gameServer: {
            select: {
              ip: true,
              port: true,
            },
          },
        },
      },
    },
  });

  // One row per server, carrying its latest snapshot.
  const servers = new Map<
    string,
    {
      snapshot: (typeof clients)[number]['snapshot'];
      playTime: number;
    }
  >();

  for (const client of clients) {
    const key = `${client.snapshot.gameServer.ip}:${client.snapshot.gameServer.port}`;
    const server = servers.get(key);
    const playTime = client.inGame ? OBSERVATION_SECONDS : 0;

    if (server === undefined) {
      servers.set(key, { snapshot: client.snapshot, playTime });
    } else {
      server.playTime += playTime;

      if (client.snapshot.createdAt > server.snapshot.createdAt) {
        server.snapshot = client.snapshot;
      }
    }
  }

  const rows = [...servers.entries()].sort(
    (a, b) =>
      b[1].snapshot.createdAt.getTime() - a[1].snapshot.createdAt.getTime()
  );

  const onlineAddresses = new Set(
    player.gameServerStateClients.map(
      (client) =>
        `${client.gameServerState.gameServer.ip}:${client.gameServerState.gameServer.port}`
    )
  );

  if (servers.size === 0) {
    return (
      <p className="px-4 lg:px-8 xl:px-16 py-4 text-[#999]">
        No recent activity — only the last couple of days are shown here.
      </p>
    );
  }

  return (
    <List
      columns={[
        {
          title: 'Server',
          expand: true,
        },
        {
          title: 'Game type',
          expand: false,
        },
        {
          title: 'Map',
          expand: false,
        },
        {
          title: 'Playtime',
          expand: false,
        },
        {
          title: 'Last seen',
          expand: false,
        },
      ]}
    >
      {rows.map(([address, server]) => (
        <Fragment key={address}>
          <ListCell
            label={server.snapshot.name}
            href={{
              pathname: `/server/${encodeIp(server.snapshot.gameServer.ip)}/${
                server.snapshot.gameServer.port
              }`,
            }}
          />
          <ListCell
            label={server.snapshot.map.gameTypeName}
            href={{
              pathname: `/gametype/${encodeString(
                server.snapshot.map.gameTypeName
              )}`,
            }}
          />
          <ListCell
            label={server.snapshot.map.name}
            href={{
              pathname: `/gametype/${encodeString(
                server.snapshot.map.gameTypeName
              )}/map/${encodeString(server.snapshot.map.name)}`,
            }}
          />
          <ListCell
            alignRight
            label={formatPlayTime(BigInt(server.playTime))}
          />
          <LastSeen
            lastSeenAt={server.snapshot.createdAt}
            gameServers={
              onlineAddresses.has(address)
                ? [server.snapshot.gameServer]
                : []
            }
            className="text-right"
          />
        </Fragment>
      ))}
    </List>
  );
}
