import prisma from '../../utils/prisma';
import { JobType } from '@prisma/client';
import { formatDistanceToNow, subMinutes } from 'date-fns';
import { keyBy } from 'lodash';

export const metadata = {
  title: 'Status',
  description: 'Teerank status',
};

export const dynamic = 'force-dynamic';

export default async function Index() {
  const [masterServersAggregate, gameServersAggregate, snapshotsAggregate] =
    await prisma.$transaction([
      prisma.masterServer.aggregate({
        _max: {
          polledAt: true,
        },
      }),
      prisma.gameServer.aggregate({
        _max: {
          polledAt: true,
        },
      }),
      prisma.gameServerSnapshot.aggregate({
        _max: {
          rankedAt: true,
          playTimedAt: true,
        },
      }),
    ]);

  const jobsAggregate = await prisma.job.groupBy({
    by: 'jobType',
    _count: {
      id: true,
    },
    orderBy: {
      jobType: 'asc',
    },
  });

  const masterServers = await prisma.masterServer.findMany({
    select: {
      address: true,
      port: true,

      _count: {
        select: {
          gameServers: true,
        },
      },
    },
    orderBy: [{
      address: 'asc',
    }, {
      port: 'asc',
    }],
  });

  const [
    unreferencedGameServersCount,
    snapshotsCount,
    rankableSnapshotsCount,
    rankedSnapshotsCount,
    playTimedSnapshotsCount,
  ] = await prisma.$transaction([
    prisma.gameServer.count({
      where: {
        masterServer: null,
      },
    }),
    prisma.gameServerSnapshot.count(),
    prisma.gameServerSnapshot.count({
      where: {
        map: {
          gameType: {
            rankMethod: {
              not: null,
            },
          },
        },
      },
    }),
    prisma.gameServerSnapshot.count({
      where: {
        map: {
          gameType: {
            rankMethod: {
              not: null,
            },
          },
        },
        rankedAt: {
          not: null,
        },
      },
    }),
    prisma.gameServerSnapshot.count({
      where: {
        playTimedAt: {
          not: null,
        },
      },
    }),
  ]);

  const jobCountByJobtype = keyBy(
    jobsAggregate,
    (jobAggregate) => jobAggregate.jobType
  );

  const sections = [
    {
      title: 'Polling Master Servers',
      date: masterServersAggregate._max.polledAt,
      jobCount: jobCountByJobtype[JobType.POLL_MASTER_SERVERS]?._count?.id ?? 0,
    },
    {
      title: 'Polling Game Servers',
      date: gameServersAggregate._max.polledAt,
      jobCount: jobCountByJobtype[JobType.POLL_GAME_SERVERS]?._count?.id ?? 0,
    },
    {
      title: 'Ranking',
      date: snapshotsAggregate._max.rankedAt,
      jobCount: jobCountByJobtype[JobType.RANK_PLAYERS]?._count?.id ?? 0,
    },
    {
      title: 'Playtiming',
      date: snapshotsAggregate._max.playTimedAt,
      jobCount: jobCountByJobtype[JobType.UPDATE_PLAYTIME]?._count?.id ?? 0,
    },
  ];

  return (
    <main className="py-12 px-20 text-[#666] flex flex-col gap-4">
      <h1 className="text-2xl font-bold clear-both">Teerank</h1>
      <div className="flex flex-col divide-y">
        {sections.map((section) => {
          const isOk =
            section.date !== null && section.date > subMinutes(new Date(), 10);

          return (
            <div key={section.title} className="flex flex-row items-center p-2">
              <span className="grow px-4">{section.title}</span>
              <div className="flex flex-row divide-x">
                {section.jobCount > 0 && <span className="text-sm text-[#d1a4a4] px-4">
                  {section.jobCount} jobs pending
                </span>}
                {section.date !== null && (
                  <span className="text-sm text-[#aaa] px-4">
                    {formatDistanceToNow(section.date, {
                      addSuffix: true,
                    })}
                  </span>
                )}
                {!isOk && (
                  <span className="font-bold text-[#b05656] px-4">Error</span>
                )}
                {isOk && (
                  <span className="font-bold text-[#5a8d39] px-4">OK</span>
                )}
              </div>
            </div>
          );
        })}
        <div className="flex flex-row items-center p-4 gap-4 text-sm text-[#aaa] text-center">
          <span className="grow">
            {`${Intl.NumberFormat('en-US', {
              maximumFractionDigits: 0,
            }).format(snapshotsCount)} snapshots`}
          </span>
          <span className="grow">
            {`${Intl.NumberFormat('en-US', {
              maximumFractionDigits: 1,
            }).format(
              (playTimedSnapshotsCount / snapshotsCount) * 100.0
            )}% play timed`}
          </span>
          <span className="grow">
            {`${Intl.NumberFormat('en-US', {
              maximumFractionDigits: 1,
            }).format(
              (rankedSnapshotsCount / rankableSnapshotsCount) * 100.0
            )}% ranked`}
          </span>
        </div>
      </div>
      <h1 className="text-2xl font-bold clear-both">Teeworlds</h1>
      <div className="flex flex-col divide-y">
        {masterServers.map((masterServer) => (
          <div
            key={masterServer.address}
            className="flex flex-row items-center p-2"
          >
            <span className="grow px-4">
              {masterServer.address}:{masterServer.port}
            </span>
            <div className="flex flex-row divide-x">
              <span className="text-sm text-[#aaa] px-4">
                {masterServer._count.gameServers} servers
              </span>
            </div>
          </div>
        ))}
        <div className="flex flex-row items-center p-2">
          <span className="grow px-4 text-[#aaa]">Unreferenced</span>
          <div className="flex flex-row divide-x">
            <span className="text-sm text-[#aaa] px-4">
              {unreferencedGameServersCount} servers
            </span>
          </div>
        </div>
      </div>
    </main>
  );
}
