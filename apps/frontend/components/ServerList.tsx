import { Fragment } from 'react';
import { formatInteger } from '../utils/format';
import { List, ListCell } from './List';
import { encodeIp, encodeString } from '../utils/encoding';

export function ServerList({
  servers,
  serverCount,
}: {
  servers: {
    rank: number;
    ip: string;
    port: number;
    name: string;
    gameTypeName: string;
    mapName: string;
    numClients: number;
    maxClients: number;
  }[];
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
      {servers.map((server) => (
        <Fragment key={`${server.ip}-${server.port}`}>
          <ListCell alignRight label={formatInteger(server.rank)} />
          <ListCell
            label={server.name || '<empty name>'}
            href={{
              pathname: `/server/${encodeIp(server.ip)}/${server.port}`,
            }}
            className={server.name === '' ? 'italic' : undefined}
          />
          <ListCell
            label={server.gameTypeName}
            href={{
              pathname: `/gametype/${encodeString(
                server.gameTypeName
              )}/servers`,
            }}
          />
          <ListCell
            label={server.mapName ?? ''}
            href={{
              pathname: `/gametype/${encodeString(
                server.gameTypeName
              )}/map/${encodeString(server.mapName)}/servers`,
            }}
          />
          <ListCell
            alignRight
            label={`${formatInteger(server.numClients)} / ${formatInteger(
              server.maxClients
            )}`}
          />
        </Fragment>
      ))}
    </List>
  );
}
