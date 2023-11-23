import { List, ListCell } from '@teerank/frontend/components/List';

export function ServerList({
  servers,
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
}) {
  return (
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
          <ListCell alignRight label={`${server.rank}`} />
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
            label={`${server.numClients} / ${server.maxClients}`}
          />
        </>
      ))}
    </List>
  );
}
