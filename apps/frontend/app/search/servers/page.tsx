import { searchParamSchema } from '../schema';
import { LayoutTabs } from '../LayoutTabs';
import { Error } from '../SearchError';
import { ServerList } from '../../../components/ServerList';
import { GameServer, GameServerSnapshot, Map } from '@prisma/client';

export const metadata = {
  title: 'Search - Servers',
  description: 'Search for servers',
};

export default function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { query } = searchParamSchema.parse(searchParams);

  const gameServerSnapshots: (GameServerSnapshot & {
    map: Map;
    gameServerLast: GameServer;
  })[] = [];

  if (query.length < 2) {
    return <Error message="Please enter at least 2 characters." />;
  }

  return (
    <LayoutTabs
      query={query}
      selectedTab="servers"
      playerCount={0}
      clanCount={0}
      gameServerCount={gameServerSnapshots.length}
    >
      <ServerList
        serverCount={gameServerSnapshots.length}
        servers={gameServerSnapshots.map((gameServerSnapshot, index) => ({
          rank: index + 1,
          name: gameServerSnapshot.name,
          gameTypeName: gameServerSnapshot.map.gameTypeName,
          mapName: gameServerSnapshot.map.name,
          ip: gameServerSnapshot.gameServerLast!.ip,
          port: gameServerSnapshot.gameServerLast!.port,
          numClients: gameServerSnapshot.numClients,
          maxClients: gameServerSnapshot.maxClients,
        }))}
      />
    </LayoutTabs>
  );
}
