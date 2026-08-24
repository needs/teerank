import { RankMethod } from '@prisma/client';
import { Fragment, Suspense } from 'react';
import Link from 'next/link';
import { twMerge } from 'tailwind-merge';
import { formatInteger, formatPlayTime } from '../utils/format';
import { List, ListCell } from './List';
import { LastSeen } from './LastSeen';
import { SharedRatio } from './SharedRatio';
import { EyeToggle } from './EyeToggle';
import { encodeString } from '../utils/encoding';

export function PlayerList({
  players,
  rankMethod,
  playerCount,
  showLastSeen,
  showInactiveToggle,
}: {
  players: {
    rank: number;
    name: string;
    clan?: string;
    isActiveClan?: boolean;
    rating?: number;
    playTime: bigint;
    lastSeenAt: Date;
    pollCount: number;
    occurrenceCount: number;
    gameServers: {
      ip: string;
      port: number;
    }[];
  }[];
  rankMethod: RankMethod | null;
  playerCount?: number;
  showLastSeen?: boolean;
  showInactiveToggle?: boolean;
}) {
  const columns: { title: React.ReactNode; expand: boolean }[] = [
    {
      title: '',
      expand: false,
    },
    {
      title: (
        <>
          Name
          <Suspense>
            <EyeToggle
              label="shared"
              title="shared names"
              param="shared"
              value="hidden"
              visibleWhenSet={false}
              className="ml-3"
            />
            {showInactiveToggle && (
              <EyeToggle
                label="inactive"
                title="past members"
                param="past"
                value="true"
                visibleWhenSet={true}
                className="ml-2"
              />
            )}
          </Suspense>
        </>
      ),
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
          <span className={twMerge('truncate', rowClassName)}>
            <Link
              prefetch={false}
              className="hover:underline"
              href={{
                pathname: `/player/${encodeString(player.name)}`,
              }}
            >
              {player.isActiveClan === false
                ? `${player.name} (Past)`
                : player.name}
            </Link>
            <SharedRatio
              pollCount={player.pollCount}
              occurrenceCount={player.occurrenceCount}
              className="ml-2"
            />
          </span>
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
          <ListCell
            alignRight
            label={formatPlayTime(player.playTime)}
            className={rowClassName}
            href={{
              pathname: `/player/${encodeString(player.name)}`,
            }}
          />
          {showLastSeen && (
            <LastSeen
              lastSeenAt={player.lastSeenAt}
              gameServers={player.gameServers}
              playerName={player.name}
              className={rowClassName}
            />
          )}
        </Fragment>
        );
      })}
    </List>
  );
}
