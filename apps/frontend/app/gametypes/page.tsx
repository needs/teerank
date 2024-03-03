import { searchParamSchema } from '../(lists)/schema';
import { List, ListCell } from '../../components/List';
import { encodeString } from '../../utils/encoding';
import { capitalize, formatInteger, formatPlayTime } from '../../utils/format';
import prisma from '../../utils/prisma';
import { Fragment } from 'react';

export const metadata = {
  title: 'Gametypes',
  description: 'List of all gametypes',
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamSchema.parse(searchParams);

  const [gameTypes, gameTypeCount] = await Promise.all([
    prisma.gameType.findMany({
      select: {
        name: true,
        rankMethod: true,
        playTime: true,
        playerCount: true,
        mapCount: true,
        clanCount: true,
        gameServerCount: true,
      },
      orderBy: [
        {
          playerCount: 'desc',
        },
        {
          mapCount: 'desc',
        },
      ],
      take: 100,
      skip: (page - 1) * 100,
    }),

    prisma.gameType.count(),
  ]);

  return (
    <div className="py-8">
      <List
        pageCount={Math.ceil(gameTypeCount / 100)}
        columns={[
          {
            title: '',
            expand: false,
          },
          {
            title: 'Name',
            expand: true,
          },
          {
            title: 'Ranking',
            expand: false,
          },
          {
            title: 'Players',
            expand: false,
          },
          {
            title: 'Clans',
            expand: false,
          },
          {
            title: 'Servers',
            expand: false,
          },
          {
            title: 'Maps',
            expand: false,
          },
          {
            title: 'Play Time',
            expand: false,
          },
        ]}
      >
        {gameTypes.map((gameType, index) => (
          <Fragment key={gameType.name}>
            <ListCell
              alignRight
              label={formatInteger((page - 1) * 100 + index + 1)}
            />
            <ListCell
              label={gameType.name}
              href={{
                pathname: `/gametype/${encodeString(gameType.name)}`,
              }}
            />
            <ListCell
              alignRight
              label={capitalize(gameType.rankMethod ?? '')}
            />
            <ListCell
              alignRight
              label={formatInteger(gameType.playerCount)}
              href={{
                pathname: `/gametype/${encodeString(gameType.name)}`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(gameType.clanCount)}
              href={{
                pathname: `/gametype/${encodeString(
                  gameType.name
                )}/clans`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(
                gameType.gameServerCount
              )}
              href={{
                pathname: `/gametype/${encodeString(
                  gameType.name
                )}/servers`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(gameType.mapCount)}
              href={{
                pathname: `/gametype/${encodeString(gameType.name)}/maps`,
              }}
            />
            <ListCell
              alignRight
              label={formatPlayTime(gameType.playTime)}
            />
          </Fragment>
        ))}
      </List>
    </div>
  );
}
