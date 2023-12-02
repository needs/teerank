import { RankMethod } from "@prisma/client";
import { formatInteger, formatPlayTime } from "../utils/format";
import { List, ListCell } from "./List";

export function PlayerList({
  players,
  rankMethod,
  playerCount,
}: {
  players: {
    rank: number;
    name: string;
    clan?: string;
    rating?: number;
    playTime: number;
  }[];
  rankMethod: RankMethod | null;
  playerCount?: number;
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
      title: 'Clan',
      expand: true,
    },
    {
      title: 'Play Time',
      expand: false,
    },
  ];

  if (rankMethod === RankMethod.ELO) {
    columns.splice(3, 0, {
      title: 'Elo',
      expand: false,
    });
  } else if (rankMethod === RankMethod.TIME) {
    columns.splice(3, 0, {
      title: 'Time',
      expand: false,
    });
  }

  return (
    <List
      columns={columns}
      pageCount={
        playerCount === undefined ? undefined : Math.ceil(playerCount / 100)
      }
    >
      {players.map((player) => (
        <>
          <ListCell alignRight label={formatInteger(player.rank)} />
          <ListCell
            label={player.name}
            href={{
              pathname: `/player/${encodeURIComponent(player.name)}`,
            }}
          />
          <ListCell
            label={player.clan ?? ''}
            href={
              player.clan === undefined
                ? undefined
                : {
                    pathname: `/clan/${encodeURIComponent(player.clan)}`,
                  }
            }
          />
          {rankMethod === RankMethod.ELO && (
            <ListCell
              alignRight
              label={
                player.rating === undefined ? '' : formatInteger(player.rating)
              }
            />
          )}
          {rankMethod === RankMethod.TIME && (
            <ListCell
              alignRight
              label={
                player.rating === undefined ? '' : formatPlayTime(-player.rating)
              }
            />
          )}
          <ListCell alignRight label={formatPlayTime(player.playTime)} />
        </>
      ))}
    </List>
  );
}
