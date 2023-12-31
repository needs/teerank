import { formatInteger, formatPlayTime } from '../utils/format';
import { List, ListCell } from './List';

export function GameTypeList({
  gameTypes,
  gameTypeCount,
}: {
  gameTypes: {
    rank: number;
    name: string;
    playTime: number;
  }[];
  gameTypeCount: number;
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
          title: 'Play Time',
          expand: false,
        },
      ]}
      pageCount={Math.ceil(gameTypeCount / 100)}
    >
      {gameTypes.map((gameType) => (
        <>
          <ListCell alignRight label={formatInteger(gameType.rank)} />
          <ListCell
            label={gameType.name}
            href={{
              pathname: `/gametype/${encodeURIComponent(gameType.name)}`,
            }}
          />
          <ListCell alignRight label={formatPlayTime(gameType.playTime)} />
        </>
      ))}
    </List>
  );
}
