import { searchParamSchema } from '../schema';
import { LayoutTabs } from '../LayoutTabs';
import { Error } from '../SearchError';
import { ClanList } from '../../../components/ClanList';
import { search } from '../../../utils/search';

export const metadata = {
  title: 'Search - Clans',
  description: 'Search for clans',
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
    <LayoutTabs query={query} selectedTab="clans" playerCount={players.length} clanCount={clans.length} gameServerCount={gameServers.length}>
      <ClanList
        clanCount={clans.length}
        clans={clans.map((clan, index) => ({
          rank: index + 1,
          name: clan.name,
          playerCount: clan.playerCount,
          playTime: BigInt(clan.playTime),
        }))}
      />
    </LayoutTabs>
  );
}
