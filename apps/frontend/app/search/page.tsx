import { searchParamSchema } from './schema';
import { LayoutTabs } from './LayoutTabs';
import { Error } from './SearchError';
import { PlayerList } from '../../components/PlayerList';
import { search } from '../../utils/search';
import { sharedHiddenParam } from '../../utils/shared';

export const metadata = {
  title: 'Search - Players',
  description: 'Search for players',
  alternates: {
    canonical: 'https://teerank.io/search',
  },
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { query } = searchParamSchema.parse(searchParams);

  if (query.length < 2) {
    return <Error message="Please enter at least 2 characters." />;
  }

  const { players, clans, gameServers } = await search(query, {
    includeShared: !sharedHiddenParam(searchParams),
  });

  return (
    <LayoutTabs query={query} selectedTab="players" playerCount={players.length} clanCount={clans.length} gameServerCount={gameServers.length}>
      <PlayerList
        playerCount={players.length}
        rankMethod={null}
        showLastSeen={true}
        players={players.map((player, index) => ({
          rank: index + 1,
          name: player.name,
          clan: player.clanName ?? undefined,
          rating: undefined,
          playTime: BigInt(player.playTime),
          lastSeenAt: player.lastSeenAt,
          pollCount: player.pollCount,
          occurrenceCount: player.occurrenceCount,
          gameServers: player.servers,
        }))}
      />
    </LayoutTabs>
  );
}
