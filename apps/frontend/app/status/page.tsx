import prisma from '../../utils/prisma';
import { formatDistanceToNow, subMinutes } from 'date-fns';

export const metadata = {
  title: 'Status',
  description: 'Teerank status',
};

export const dynamic = 'force-dynamic';

export default async function Index() {
  const masterServersAggregate = await prisma.masterServer.aggregate({
    _max: {
      polledAt: true,
    },
  });

  const masterServerPollingJobs = await prisma.masterServer.count({
    where: {
      pollingStartedAt: {
        not: null,
      },
    },
  });

  const gameServersAggregate = await prisma.gameServer.aggregate({
    _max: {
      polledAt: true,
    },
  });

  const gameServerPollingJobs = await prisma.gameServer.count({
    where: {
      pollingStartedAt: {
        not: null,
      },
    },
  });

  const snapshotsAggregate = await prisma.gameServerSnapshot.aggregate({
    _max: {
      rankedAt: true,
      playTimedAt: true,
    },
  });

  const snapshotRankingJobs = await prisma.gameServerSnapshot.count({
    where: {
      rankingStartedAt: {
        not : null
      }
    },
  });

  const snapshotPlayTimingJobs = await prisma.gameServerSnapshot.count({
    where: {
      playTimingStartedAt: {
        not : null
      }
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
    orderBy: [
      {
        address: 'asc',
      },
      {
        port: 'asc',
      },
    ],
  });

  const unreferencedGameServersCount = await prisma.gameServer.count({
    where: {
      masterServer: null,
    },
  });

  const sections = [
    {
      title: 'Polling Master Servers',
      date: masterServersAggregate._max.polledAt,
      jobCount: masterServerPollingJobs,
    },
    {
      title: 'Polling Game Servers',
      date: gameServersAggregate._max.polledAt,
      jobCount: gameServerPollingJobs,
    },
    {
      title: 'Ranking',
      date: snapshotsAggregate._max.rankedAt,
      jobCount: snapshotRankingJobs,
    },
    {
      title: 'Playtiming',
      date: snapshotsAggregate._max.playTimedAt,
      jobCount: snapshotPlayTimingJobs,
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
                {section.jobCount > 0 && (
                  <span className="text-sm text-[#d1a4a4] px-4">
                    {section.jobCount} jobs pending
                  </span>
                )}
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
