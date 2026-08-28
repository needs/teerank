import { paramsSchema } from '../schema';
import { z } from 'zod';
import { notFound } from 'next/navigation';
import { Metadata } from 'next';
import Link from 'next/link';
import { Fragment } from 'react';
import { intervalToDuration } from 'date-fns';
import prisma from '../../../../utils/prisma';
import { searchParamPageSchema } from '../../../../utils/page';
import { List, ListCell } from '../../../../components/List';
import { durationClassName, LastSeen } from '../../../../components/LastSeen';
import { SharedRatio } from '../../../../components/SharedRatio';
import { DDNetAttribution } from '../../../../components/DDNetAttribution';
import { encodeString } from '../../../../utils/encoding';
import {
  formatDurationShort,
  formatFinishTime,
  formatInteger,
  formatPlayTime,
} from '../../../../utils/format';
import { countTeammates, getTeammates } from '../../../../utils/teammates';
import {
  countDdnetPartnerMaps,
  countDdnetPartners,
  formatUtcDate,
  getDdnetPartnerMaps,
  getDdnetPartners,
} from '../../../../utils/ddnetPlayer';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { playerName } = paramsSchema.parse(params);

  return {
    title: `Player ${playerName} - Teammates`,
    description: 'Players a Teeworlds player spent the most time with',
    alternates: {
      canonical: `https://teerank.io/player/${encodeString(playerName)}/teammates`,
    },
  };
}

function mapHref(mapName: string) {
  return {
    pathname: `/gametype/DDraceNetwork/map/${encodeString(mapName)}`,
  };
}

function SourceToggle({
  pathname,
  label,
  query,
}: {
  pathname: string;
  label: string;
  query?: { [key: string]: string };
}) {
  return (
    <Link
      prefetch={false}
      href={{ pathname, query }}
      className="ml-3 rounded border border-[#970] px-2 align-middle text-sm font-normal hover:bg-[#970] hover:text-white"
    >
      {label}
    </Link>
  );
}

async function PartnerMapList({
  playerId,
  playerName,
  partnerName,
  page,
}: {
  playerId: number;
  playerName: string;
  partnerName: string;
  page: number;
}) {
  const partner = await prisma.player.findUnique({
    select: {
      id: true,
    },
    where: {
      name: partnerName,
    },
  });

  if (partner === null) {
    return notFound();
  }

  const pathname = `/player/${encodeString(playerName)}/teammates`;

  const [rows, count] = await Promise.all([
    getDdnetPartnerMaps(playerId, partner.id, {
      skip: (page - 1) * 100,
      take: 100,
    }),
    countDdnetPartnerMaps(playerId, partner.id),
  ]);

  return (
    <>
      <div className="flex flex-row items-baseline gap-4 px-4 lg:px-8 xl:px-16">
        <h2 className="text-xl font-bold">
          with{' '}
          <Link
            prefetch={false}
            className="hover:underline"
            href={{ pathname: `/player/${encodeString(partnerName)}` }}
          >
            {partnerName}
          </Link>
        </h2>
        <Link
          prefetch={false}
          className="text-sm text-[#970] hover:underline"
          href={{ pathname, query: { source: 'finishes' } }}
        >
          Back to partners
        </Link>
      </div>
      <List
        columns={[
          { title: 'Map', expand: true },
          { title: 'Best time', expand: false },
          { title: 'Finishes', expand: false },
          { title: 'Duo rank', expand: false },
          { title: 'Last finish', expand: false },
        ]}
        pageCount={Math.ceil(count / 100)}
      >
        {rows.map((row) => (
          <Fragment key={row.mapId}>
            <ListCell
              label={row.mapName ?? '—'}
              href={row.mapName === null ? undefined : mapHref(row.mapName)}
            />
            <ListCell alignRight label={formatFinishTime(row.bestTime)} />
            <ListCell alignRight label={formatInteger(row.finishCount)} />
            <ListCell alignRight label={formatInteger(row.duoRank)} />
            <ListCell alignRight label={formatUtcDate(row.lastFinishAt)} />
          </Fragment>
        ))}
      </List>
      <DDNetAttribution />
    </>
  );
}

async function PartnerList({
  playerId,
  playerName,
  page,
}: {
  playerId: number;
  playerName: string;
  page: number;
}) {
  const pathname = `/player/${encodeString(playerName)}/teammates`;

  const [partners, partnerCount] = await Promise.all([
    getDdnetPartners(playerId, { skip: (page - 1) * 100, take: 100 }),
    countDdnetPartners(playerId),
  ]);

  return (
    <>
      <List
        columns={[
          {
            title: (
              <>
                Name
                <SourceToggle pathname={pathname} label="Polled" />
              </>
            ),
            expand: true,
          },
          { title: 'Finishes together', expand: false },
          { title: 'Best time', expand: true },
          { title: 'Last together', expand: false },
        ]}
        pageCount={Math.ceil(partnerCount / 100)}
      >
        {partners.map((partner) => (
          <Fragment key={partner.name}>
            <span className="truncate">
              <Link
                prefetch={false}
                className="hover:underline"
                href={{ pathname: `/player/${encodeString(partner.name)}` }}
              >
                {partner.name}
              </Link>
              <SharedRatio
                pollCount={partner.pollCount}
                occurrenceCount={partner.occurrenceCount}
                className="ml-2"
              />
            </span>
            <ListCell
              alignRight
              label={formatInteger(partner.finishCount)}
              href={{
                pathname,
                query: { source: 'finishes', with: partner.name },
              }}
            />
            <span className="truncate">
              {formatFinishTime(partner.bestTime)}
              {partner.bestMapName !== null && (
                <>
                  {' · '}
                  <Link
                    prefetch={false}
                    className="hover:underline"
                    href={mapHref(partner.bestMapName)}
                  >
                    {partner.bestMapName}
                  </Link>
                </>
              )}
            </span>
            <ListCell alignRight label={formatUtcDate(partner.lastFinishAt)} />
          </Fragment>
        ))}
      </List>
      <DDNetAttribution />
    </>
  );
}

export default async function Index({
  params,
  searchParams,
}: {
  params: z.infer<typeof paramsSchema>;
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { playerName } = paramsSchema.parse(params);
  const { page } = searchParamPageSchema.parse(searchParams);
  const source = searchParams['source'] === 'finishes' ? 'finishes' : 'polled';
  const withName =
    typeof searchParams['with'] === 'string' ? searchParams['with'] : undefined;

  const player = await prisma.player.findUnique({
    select: {
      id: true,
    },
    where: {
      name: playerName,
    },
  });

  if (player === null) {
    return notFound();
  }

  if (source === 'finishes') {
    if (withName !== undefined) {
      return (
        <PartnerMapList
          playerId={player.id}
          playerName={playerName}
          partnerName={withName}
          page={page}
        />
      );
    }

    return (
      <PartnerList playerId={player.id} playerName={playerName} page={page} />
    );
  }

  const pathname = `/player/${encodeString(playerName)}/teammates`;

  const [teammates, teammateCount, partnerCount] = await Promise.all([
    getTeammates(player.id, { skip: (page - 1) * 100, take: 100 }),
    countTeammates(player.id),
    countDdnetPartners(player.id),
  ]);

  return (
    <List
      columns={[
        {
          title: (
            <>
              Name
              {partnerCount > 0 && (
                <SourceToggle
                  pathname={pathname}
                  label="Finishes"
                  query={{ source: 'finishes' }}
                />
              )}
            </>
          ),
          expand: true,
        },
        { title: 'Clan', expand: true },
        { title: 'Last seen', expand: false },
        { title: 'Coplaytime', expand: false },
        { title: 'Last together', expand: false },
      ]}
      pageCount={Math.ceil(teammateCount / 100)}
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
