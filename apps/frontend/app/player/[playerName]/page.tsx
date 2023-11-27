import prisma from '@teerank/frontend/utils/prisma';
import { paramsSchema } from './schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import Link from 'next/link';
import Image from 'next/image';
import { formatInteger, formatPlayTime } from '@teerank/frontend/utils/format';
import { List, ListCell } from '@teerank/frontend/components/List';
import { searchParamPageSchema } from '@teerank/frontend/utils/page';

export const metadata = {
  title: 'Player',
  description: 'A Teeworlds player',
};

export default async function Index({
  params,
  searchParams,
}: {
  params: z.infer<typeof paramsSchema>;
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const parsedParams = paramsSchema.parse(params);
  const { page } = searchParamPageSchema.parse(searchParams);

  const player = await prisma.player.findUnique({
    select: {
      name: true,
      clanName: true,
      playTime: true,

      _count: {
        select: {
          playerInfos: {
            where: {
              gameType: {
                isNot: null,
              },
            },
          },
        },
      },

      playerInfos: {
        select: {
          gameTypeName: true,
          playTime: true,
        },
        where: {
          gameType: {
            isNot: null,
          },
        },
      },
    },
    where: {
      name: parsedParams.playerName,
    },
  });

  if (player === null) {
    return notFound();
  }

  return (
    <main className="flex flex-col gap-8 py-12">
      <header className="flex flex-row px-20 gap-4 items-center">
        <Image src="/player.png" width={100} height={100} alt="Player" />
        <section className="flex flex-col gap-2 grow">
          <h1 className="text-2xl font-bold">{player.name}</h1>
          <span className="pr-4">
            {player.clanName !== null && (
              <Link
                className="hover:underline"
                href={{
                  pathname: `/clan/${encodeURIComponent(player.clanName)}`,
                }}
              >
                {player.clanName}
              </Link>
            )}
          </span>
        </section>
        <aside className="flex flex-col gap-2 text-right">
          <p>
            <b>Playtime</b>: {formatPlayTime(player.playTime)}
          </p>
          <p>7 hours ago</p>
        </aside>
      </header>
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
        pageCount={1}
      >
        {player.playerInfos.map((playerInfo, index) => (
          <>
            <ListCell
              alignRight
              label={formatInteger((page - 1) * 100 + index + 1)}
            />
            <ListCell
              label={playerInfo.gameTypeName ?? ''}
              href={{
                pathname: `/gametype/${encodeURIComponent(
                  playerInfo.gameTypeName ?? ''
                )}`,
              }}
            />
            <ListCell alignRight label={formatPlayTime(playerInfo.playTime)} />
          </>
        ))}
      </List>
    </main>
  );
}
