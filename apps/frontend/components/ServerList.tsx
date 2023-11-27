import { List, ListCell } from '@teerank/frontend/components/List';
import { formatInteger } from '../utils/format';

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
        <>
          <ListCell alignRight label={formatInteger(server.rank)} />
          <ListCell
            label={server.name}
            href={{
              pathname: `/server/${encodeURIComponent(
                server.ip
              )}/${encodeURIComponent(server.port)}`,
            }}
          />
          <ListCell
            label={server.gameTypeName}
            href={{
              pathname: `/gametype/${encodeURIComponent(
                server.gameTypeName
              )}/servers`,
            }}
          />
          <ListCell
            label={server.mapName ?? ''}
            href={{
              pathname: `/gametype/${encodeURIComponent(
                server.gameTypeName
              )}/map/${encodeURIComponent(server.mapName)}/servers`,
            }}
          />
          <ListCell
            alignRight
            label={`${formatInteger(server.numClients)} / ${formatInteger(
              server.maxClients
            )}`}
          />
        </>
      ))}
    </List>
  );
}
