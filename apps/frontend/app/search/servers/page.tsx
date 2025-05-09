import { searchParamSchema } from '../schema';
import { LayoutTabs } from '../LayoutTabs';
import { Error } from '../SearchError';
import { ServerList } from '../../../components/ServerList';
import { search } from '../../../utils/search';

export const metadata = {
  title: 'Search - Servers',
  description: 'Search for servers',
  alternates: {
    canonical: 'https://teerank.io/search/servers',
  },
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { query } = searchParamSchema.parse(searchParams);
  const { players, clans, gameServers } = await search(query);

  if (query.length < 2) {
    return <Error message="Please enter at least 2 characters." />;
  }

  return (
    <LayoutTabs
      query={query}
      selectedTab="servers"
      playerCount={players.length}
      clanCount={clans.length}
      gameServerCount={gameServers.length}
    >
      <ServerList
        serverCount={gameServers.length}
        servers={gameServers.map((gameServer, index) => ({
          rank: index + 1,
          ip: gameServer.ip,
          port: gameServer.port,
          state: {
            name: gameServer.name,
            gameTypeName: gameServer.gameTypeName,
            mapName: gameServer.mapName,
            numClients: gameServer.numClients,
            maxClients: gameServer.maxClients,
          },
        }))}
      />
    </LayoutTabs>
  );
}
