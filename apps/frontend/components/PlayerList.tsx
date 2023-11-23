import { List, ListCell } from '@teerank/frontend/components/List';
import { formatPlayTime, formatInteger } from '@teerank/frontend/utils/format';
import { format } from 'path';

export function PlayerList({
  players,
  hideRating,
}: {
  players: {
    rank: number;
    name: string;
    clan?: string;
    rating?: number;
    playTime: number;
  }[];
  hideRating?: boolean;
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
      title: 'Elo',
      expand: false,
    },
    {
      title: 'Play Time',
      expand: false,
    },
  ];

  if (hideRating) {
    columns.splice(3, 1);
  }

  return (
    <List columns={columns}>
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
          {!hideRating && (
            <ListCell
              alignRight
              label={
                player.rating === undefined ? '' : formatInteger(player.rating)
              }
            />
          )}
          <ListCell alignRight label={formatPlayTime(player.playTime)} />
        </>
      ))}
    </List>
  );
}
