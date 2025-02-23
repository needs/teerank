import { Fragment } from 'react';
import { formatInteger } from '../utils/format';
import { List, ListCell } from './List';
import { encodeIp, encodeString } from '../utils/encoding';

type OnlineServer = {
  rank: number;
  ip: string;
  port: number;
  state: {
    name: string;
    gameTypeName: string;
    mapName: string;
    numClients: number;
    maxClients: number;
  };
};

type OfflineServer = {
  rank: number;
  ip: string;
  port: number;
  state: null;
};

type Server = OnlineServer | OfflineServer;

function OnlineServerRow({ server }: { server: OnlineServer }) {
  return (
    <>
      <ListCell alignRight label={formatInteger(server.rank)} />
      <ListCell
        label={server.state.name || '<empty name>'}
        href={{
          pathname: `/server/${encodeIp(server.ip)}/${server.port}`,
        }}
        className={server.state.name === '' ? 'italic' : undefined}
      />
      <ListCell
        label={server.state.gameTypeName}
        href={{
          pathname: `/gametype/${encodeString(
            server.state.gameTypeName
          )}/servers`,
        }}
      />
      <ListCell
        label={server.state.mapName ?? ''}
        href={{
          pathname: `/gametype/${encodeString(
            server.state.gameTypeName
          )}/map/${encodeString(server.state.mapName)}/servers`,
        }}
      />
      <ListCell
        alignRight
        label={`${formatInteger(server.state.numClients)} / ${formatInteger(
          server.state.maxClients
        )}`}
      />
    </>
  );
}

function OfflineServerRow({ server }: { server: OfflineServer }) {
  return (
    <>
      <ListCell alignRight label={formatInteger(server.rank)} />
      <ListCell
        label={`${server.ip}:${server.port}`}
        href={{
          pathname: `/server/${encodeIp(server.ip)}/${server.port}`,
        }}
        className="italic text-[#999]"
      />
      <ListCell />
      <ListCell />
      <ListCell />
    </>
  );
}

export function ServerList({
  servers,
  serverCount,
}: {
  servers: Server[];
  serverCount?: number;
}) {
  return (
    <List
      pageCount={
        serverCount === undefined ? undefined : Math.ceil(serverCount / 100)
      }
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
          title: 'Game Type',
          expand: false,
        },
        {
          title: 'Map',
          expand: false,
        },
        {
          title: 'Players',
          expand: false,
        },
      ]}
    >
      {servers.map((server) => {
        if (server.state !== null) {
          return (
            <OnlineServerRow
              key={`${server.ip}-${server.port}`}
              server={server}
            />
          );
        } else {
          return (
            <OfflineServerRow
              key={`${server.ip}-${server.port}`}
              server={server}
            />
          );
        }
      })}
    </List>
  );
}
