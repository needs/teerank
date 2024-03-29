import { List, ListCell } from '../../../../components/List';
import { formatInteger, formatPlayTime } from '../../../../utils/format';
import prisma from '../../../../utils/prisma';
import { paramsSchema, searchParamsSchema } from '../schema';
import { notFound } from 'next/navigation';
import { encodeString } from '../../../../utils/encoding';

export const metadata = {
  title: 'Maps',
  description: 'List of gametype maps',
};

export default async function Index({
  params,
  searchParams,
}: {
  params: { [key: string]: string };
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { page } = searchParamsSchema.parse(searchParams);
  const { gameTypeName } = paramsSchema.parse(params);

  const gameType = await prisma.gameType.findUnique({
    where: {
      name: gameTypeName,
    },
  });

  if (gameType === null) {
    return notFound();
  }

  const [maps, mapCount] = await Promise.all([
    prisma.map.findMany({
      select: {
        name: true,
        gameTypeName: true,
        playTime: true,
        playerCount: true,
        clanCount: true,
        gameServerCount: true,
      },
      where: {
        gameType: {
          name: { equals: gameTypeName },
        },
      },
      orderBy: [
        {
          playerCount: 'desc',
        },
      ],
      take: 100,
      skip: (page - 1) * 100,
    }),

    prisma.map.count({
      where: {
        gameType: {
          name: { equals: gameTypeName },
        },
      },
    }),
  ]);

  return (
    <div className="py-8">
      <List
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
            title: 'Play Time',
            expand: false,
          },
        ]}
        pageCount={Math.ceil(mapCount / 100)}
      >
        {maps.map((map, index) => (
          <>
            <ListCell
              alignRight
              label={formatInteger((page - 1) * 100 + index + 1)}
            />
            <ListCell
              label={map.name}
              href={{
                pathname: `/gametype/${encodeString(
                  map.gameTypeName
                )}/map/${encodeString(map.name)}`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(map.playerCount)}
              href={{
                pathname: `/gametype/${encodeString(
                  map.gameTypeName
                )}/map/${encodeString(map.name)}`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(map.clanCount)}
              href={{
                pathname: `/gametype/${encodeString(
                  map.gameTypeName
                )}/map/${encodeString(map.name)}/clans`,
              }}
            />
            <ListCell
              alignRight
              label={formatInteger(map.gameServerCount)}
              href={{
                pathname: `/gametype/${encodeString(
                  map.gameTypeName
                )}/map/${encodeString(map.name)}/servers`,
              }}
            />
            <ListCell alignRight label={formatPlayTime(map.playTime)} />
          </>
        ))}
      </List>
    </div>
  );
}
