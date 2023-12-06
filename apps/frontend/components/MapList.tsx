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
    playTime: number;
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
        <>
          <ListCell alignRight label={formatInteger(map.rank)} />
          <ListCell
            label={map.name}
            href={{
              pathname: `/gametype/${encodeURIComponent(map.name)}`,
            }}
          />
          <ListCell
            label={map.name}
            href={{
              pathname: `/gametype/${encodeURIComponent(map.name)}/map/${encodeURIComponent(map.name)}`,
            }}
          />
          <ListCell alignRight label={formatPlayTime(map.playTime)} />
        </>
      ))}
    </List>
  );
}
