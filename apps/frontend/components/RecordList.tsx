import { Fragment, Suspense } from 'react';
import Link from 'next/link';
import { formatFinishTime, formatInteger, formatTimeDelta } from '../utils/format';
import { List, ListCell } from './List';
import { SharedRatio } from './SharedRatio';
import { EyeToggle } from './EyeToggle';
import { encodeString } from '../utils/encoding';

export type RecordRanks = 'solo' | 'team' | 'duo';

function RanksToggle({
  ranks,
  pathname,
}: {
  ranks: RecordRanks;
  pathname: string;
}) {
  const options: { value: RecordRanks; label: string }[] = [
    { value: 'solo', label: 'Solo' },
    { value: 'team', label: 'Team' },
    { value: 'duo', label: 'Duo' },
  ];

  return (
    <span className="ml-3 inline-flex flex-row gap-2 align-middle text-sm font-normal">
      {options.map(({ value, label }) =>
        value === ranks ? (
          <span
            key={value}
            className="rounded border border-[#970] bg-[#970] px-2 text-white"
          >
            {label}
          </span>
        ) : (
          <Link
            key={value}
            prefetch={false}
            href={
              value === 'solo'
                ? { pathname }
                : { pathname, query: { ranks: value } }
            }
            className="rounded border border-[#970] px-2 hover:bg-[#970] hover:text-white"
          >
            {label}
          </Link>
        )
      )}
    </span>
  );
}

export function RecordList({
  records,
  recordCount,
  ranks,
  pathname,
  showRank,
  showMap,
  showDelta,
  showSharedToggle,
}: {
  records: {
    rank?: number;
    map?: { name: string; gameTypeName: string };
    players: { name: string; pollCount: number; occurrenceCount: number }[];
    time: number;
    delta?: number | null;
    dateLabel: string;
  }[];
  recordCount?: number;
  ranks?: RecordRanks;
  pathname?: string;
  showRank?: boolean;
  showMap?: boolean;
  showDelta?: boolean;
  showSharedToggle?: boolean;
}) {
  const columns: { title: React.ReactNode; expand: boolean }[] = [];

  if (showRank) {
    columns.push({
      title: '',
      expand: false,
    });
  }

  if (showMap) {
    columns.push({
      title: 'Map',
      expand: true,
    });
  }

  columns.push({
    title: (
      <>
        Name
        {ranks !== undefined && pathname !== undefined && (
          <RanksToggle ranks={ranks} pathname={pathname} />
        )}
        {showSharedToggle && (
          <Suspense>
            <EyeToggle
              label="shared"
              title="shared names"
              param="shared"
              value="hidden"
              visibleWhenSet={false}
              className="ml-3"
            />
          </Suspense>
        )}
      </>
    ),
    expand: true,
  });

  columns.push({
    title: 'Time',
    expand: false,
  });

  if (showDelta) {
    columns.push({
      title: 'Improved by',
      expand: false,
    });
  }

  columns.push({
    title: 'Date',
    expand: false,
  });

  return (
    <List
      columns={columns}
      pageCount={
        recordCount === undefined ? undefined : Math.ceil(recordCount / 100)
      }
    >
      {records.map((record, index) => (
        <Fragment key={index}>
          {showRank && (
            <ListCell alignRight label={formatInteger(record.rank ?? 0)} />
          )}
          {showMap && (
            <ListCell
              label={record.map?.name ?? ''}
              href={
                record.map === undefined
                  ? undefined
                  : {
                      pathname: `/gametype/${encodeString(
                        record.map.gameTypeName
                      )}/map/${encodeString(record.map.name)}/records`,
                    }
              }
            />
          )}
          <span className="truncate">
            {record.players.map((player, playerIndex) => (
              <Fragment key={player.name}>
                {playerIndex > 0 && ', '}
                <Link
                  prefetch={false}
                  className="hover:underline"
                  href={{
                    pathname: `/player/${encodeString(player.name)}`,
                  }}
                >
                  {player.name}
                </Link>
                <SharedRatio
                  pollCount={player.pollCount}
                  occurrenceCount={player.occurrenceCount}
                  className="ml-2"
                />
              </Fragment>
            ))}
          </span>
          <ListCell alignRight label={formatFinishTime(record.time)} />
          {showDelta && (
            <ListCell
              alignRight
              label={
                record.delta === null || record.delta === undefined
                  ? ''
                  : formatTimeDelta(record.delta)
              }
            />
          )}
          <ListCell alignRight label={record.dateLabel} />
        </Fragment>
      ))}
    </List>
  );
}
