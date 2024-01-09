import { formatInteger, formatPlayTime } from "../utils/format";
import { List, ListCell } from "./List";

export function ClanList({
  clans,
  clanCount,
}: {
  clans: {
    rank: number;
    name: string;
    playerCount: number;
    playTime: bigint;
  }[];
  clanCount?: number;
}) {
  const columns = [
    {
      title: '',
      expand: false,
    },
    {
      title: 'Name',
      expand: true,
    },
    {
      title: 'Players',
      expand: false,
    },
    {
      title: 'Play Time',
      expand: false,
    },
  ];

  return (
    <List
      columns={columns}
      pageCount={
        clanCount === undefined ? undefined : Math.ceil(clanCount / 100)
      }
    >
      {clans.map((clan) => (
        <>
          <ListCell alignRight label={formatInteger(clan.rank)} />
          <ListCell
            label={clan.name}
            href={{
              pathname: `/clan/${encodeURIComponent(clan.name)}`,
            }}
          />
          <ListCell alignRight label={formatInteger(clan.playerCount)} />
          <ListCell alignRight label={formatPlayTime(clan.playTime)} />
        </>
      ))}
    </List>
  );
}
