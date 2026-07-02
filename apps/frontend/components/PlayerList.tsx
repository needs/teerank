import { RankMethod } from '@prisma/client';
import { formatInteger, formatPlayTime } from '../utils/format';
import { List, ListCell } from './List';
import { LastSeen } from './LastSeen';
import { Fragment } from 'react';
import { encodeString } from '../utils/encoding';

export function PlayerList({
  players,
  rankMethod,
  playerCount,
  showLastSeen,
}: {
  players: {
    rank: number;
    name: string;
    clan?: string;
    isActiveClan?: boolean;
    rating?: number;
    playTime: bigint;
    lastSeenAt: Date;
    gameServers: {
      ip: string;
      port: number;
    }[];
  }[];
  rankMethod: RankMethod | null;
  playerCount?: number;
  showLastSeen?: boolean;
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

  if (showLastSeen) {
    columns.push({
      title: 'Last Seen',
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
      {players.map((player) => {
        const rowClassName = player.isActiveClan === false ? 'text-gray-400' : '';
        
        return (
        <Fragment key={player.name}>
          <ListCell alignRight label={formatInteger(player.rank)} className={rowClassName} />
          <ListCell
            label={player.isActiveClan === false ? `${player.name} (Past)` : player.name}
            className={rowClassName}
            href={{
              pathname: `/player/${encodeString(player.name)}`,
            }}
          />
          <ListCell
            label={player.clan ?? ''}
            className={rowClassName}
            href={
              player.clan === undefined
                ? undefined
                : {
                    pathname: `/clan/${encodeString(player.clan)}`,
                  }
            }
          />
          {rankMethod === RankMethod.ELO && (
            <ListCell
              alignRight
              className={rowClassName}
              label={
                player.rating === undefined ? '' : formatInteger(player.rating)
              }
            />
          )}
          {rankMethod === RankMethod.TIME && (
            <ListCell
              alignRight
              className={rowClassName}
              label={
                player.rating === undefined
                  ? ''
                  : formatPlayTime(BigInt(-player.rating))
              }
            />
          )}
          <ListCell alignRight label={formatPlayTime(player.playTime)} className={rowClassName} />
          {showLastSeen && (
            <LastSeen
              lastSeenAt={player.lastSeenAt}
              gameServers={player.gameServers}
              className={rowClassName}
            />
          )}
        </Fragment>
        );
      })}
    </List>
  );
}
