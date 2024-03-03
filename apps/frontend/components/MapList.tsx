import { Fragment } from 'react';
import { formatInteger, formatPlayTime } from '../utils/format';
import { List, ListCell } from './List';

export function MapList({
  maps: maps,
  mapCount: mapCount,
}: {
  maps: {
    rank: number;
    name: string;
    gameTypeName: string;
    playTime: bigint;
  }[];
  mapCount: number;
}) {
  return (
    <List
      columns={[
        {
          title: '',
          expand: false,
        },
        {
          title: 'Game type',
          expand: true,
        },
        {
          title: 'Name',
          expand: true,
        },
        {
          title: 'Play Time',
          expand: false,
        },
      ]}
      pageCount={Math.ceil(mapCount / 100)}
    >
      {maps.map((map) => (
        <Fragment key={map.name}>
          <ListCell alignRight label={formatInteger(map.rank)} />
          <ListCell
            label={map.gameTypeName}
            href={{
              pathname: `/gametype/${encodeURIComponent(map.gameTypeName)}`,
            }}
          />
          <ListCell
            label={map.name}
            href={{
              pathname: `/gametype/${encodeURIComponent(map.gameTypeName)}/map/${encodeURIComponent(map.name)}`,
            }}
          />
          <ListCell alignRight label={formatPlayTime(map.playTime)} />
        </Fragment>
      ))}
    </List>
  );
}
