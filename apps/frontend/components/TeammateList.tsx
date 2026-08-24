import { Fragment } from 'react';
import { intervalToDuration } from 'date-fns';
import { List, ListCell } from './List';
import { durationClassName, LastSeen } from './LastSeen';
import { encodeString } from '../utils/encoding';
import { formatDurationShort, formatPlayTime } from '../utils/format';
import { Teammate } from '../utils/teammates';

export function TeammateList({
  teammates,
  pageCount,
}: {
  teammates: Teammate[];
  pageCount?: number;
}) {
  return (
    <List
      columns={[
        {
          title: 'Name',
          expand: true,
        },
        {
          title: 'Clan',
          expand: true,
        },
        {
          title: 'Last seen',
          expand: false,
        },
        {
          title: 'Coplaytime',
          expand: false,
        },
        {
          title: 'Last together',
          expand: false,
        },
      ]}
      pageCount={pageCount}
    >
      {teammates.map((teammate) => {
        const lastTogether = intervalToDuration({
          start: teammate.lastPlayedAt,
          end: new Date(),
        });

        return (
          <Fragment key={teammate.name}>
            <ListCell
              label={teammate.name}
              href={{
                pathname: `/player/${encodeString(teammate.name)}`,
              }}
            />
            <ListCell
              label={teammate.clan ?? ''}
              href={
                teammate.clan === null
                  ? undefined
                  : {
                      pathname: `/clan/${encodeString(teammate.clan)}`,
                    }
              }
            />
            <LastSeen
              lastSeenAt={teammate.lastSeenAt}
              gameServers={teammate.gameServers}
              playerName={teammate.name}
            />
            <ListCell
              alignRight
              label={formatPlayTime(BigInt(teammate.playTime))}
            />
            <ListCell
              alignRight
              className={durationClassName(lastTogether)}
              label={formatDurationShort(lastTogether)}
            />
          </Fragment>
        );
      })}
    </List>
  );
}
