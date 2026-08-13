import {
  getLastUpdatePlayTimeDate,
  getLastPollGameServerDate,
  getLastRankPlayerDate,
  getLastGameTypeCountDate,
  getLastMapCountDate,
  getLastPollMasterServerDate,
  getLastArchiveSnapshotsDate,
  getArchiveSnapshotsFailedCount,
  SNAPSHOT_RETENTION_HOURS,
} from '@teerank/teerank';
import prisma from '../../utils/prisma';
import {
  formatDistanceStrict,
  formatDistanceToNow,
  subHours,
  subMinutes,
} from 'date-fns';

export const metadata = {
  title: 'Status - Teerank',
  description: 'Teerank status and statistics.',
  alternates: {
    canonical: 'https://teerank.io/status',
  },
};

export const dynamic = 'force-dynamic';

export default async function Index() {
  const [
    lastPollMasterServerDate,
    lastPollGameServerDate,
    lastPlayTimedSnapshotDate,
    lastRankedSnapshotDate,
    lastGameTypeCountDate,
    lastMapCountDate,
    lastArchiveSnapshotsDate,
    archiveSnapshotsFailedCount,
    oldestSnapshot,
    masterServers,
    unreferencedGameServersCount,
  ] = await Promise.all([
    getLastPollMasterServerDate(),
    getLastPollGameServerDate(),
    getLastUpdatePlayTimeDate(),
    getLastRankPlayerDate(),
    getLastGameTypeCountDate(),
    getLastMapCountDate(),
    getLastArchiveSnapshotsDate(),
    getArchiveSnapshotsFailedCount(),
    prisma.gameServerSnapshot.findFirst({
      orderBy: {
        id: 'asc',
      },
      select: {
        createdAt: true,
      },
    }),
    prisma.masterServer.findMany({
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
    }),
    prisma.gameServer.count({
      where: {
        masterServer: null,
      },
    })
  ]);

  const sections = [
    {
      title: 'Polling Master Servers',
      date: lastPollMasterServerDate,
      staleAfterMinutes: 10,
    },
    {
      title: 'Polling Game Servers',
      date: lastPollGameServerDate,
      staleAfterMinutes: 10,
    },
    {
      title: 'Ranking',
      date: lastRankedSnapshotDate,
      staleAfterMinutes: 10,
    },
    {
      title: 'Playtiming',
      date: lastPlayTimedSnapshotDate,
      staleAfterMinutes: 10,
    },
    {
      title: 'Game type count',
      date: lastGameTypeCountDate,
      staleAfterMinutes: 10,
    },
    {
      title: 'Map count',
      date: lastMapCountDate,
      staleAfterMinutes: 10,
    },
    {
      title: 'Archiving snapshots',
      date: lastArchiveSnapshotsDate,
      staleAfterMinutes: 30,
    },
  ];

  const retentionCutoff = subHours(new Date(), SNAPSHOT_RETENTION_HOURS);
  const archiveBacklog =
    oldestSnapshot !== null && oldestSnapshot.createdAt < retentionCutoff
      ? formatDistanceStrict(oldestSnapshot.createdAt, retentionCutoff)
      : null;

  return (
    <main className="py-12 px-4 md:px-12 xl:px-20 text-[#666] flex flex-col gap-4">
      <h1 className="text-2xl font-bold clear-both">Teerank</h1>
      <div className="flex flex-col divide-y">
        {sections.map((section) => {
          const isOk =
            section.date !== null &&
            section.date > subMinutes(new Date(), section.staleAfterMinutes);

          return (
            <div key={section.title} className="flex flex-row items-center p-2">
              <span className="grow px-4">{section.title}</span>
              <div className="flex flex-row divide-x">
                {section.date !== null && (
                  <span className="text-sm text-[#aaa] px-4">
                    {formatDistanceToNow(section.date, {
                      addSuffix: true,
                    })}
                  </span>
                )}
                {!isOk && (
                  <span className="font-bold text-[#b05656] px-4">Down</span>
                )}
                {isOk && (
                  <span className="font-bold text-[#5a8d39] px-4">OK</span>
                )}
              </div>
            </div>
          );
        })}
      </div>

      <h1 className="text-2xl font-bold clear-both">Archiving</h1>
      <div className="flex flex-col divide-y">
        <div className="flex flex-row items-center p-2">
          <span className="grow px-4">Retention</span>
          <div className="flex flex-row divide-x">
            <span className="text-sm text-[#aaa] px-4">
              {SNAPSHOT_RETENTION_HOURS} hours
            </span>
          </div>
        </div>

        <div className="flex flex-row items-center p-2">
          <span className="grow px-4">Oldest snapshot</span>
          <div className="flex flex-row divide-x">
            <span className="text-sm text-[#aaa] px-4">
              {oldestSnapshot === null
                ? 'None'
                : formatDistanceToNow(oldestSnapshot.createdAt, {
                    addSuffix: true,
                  })}
            </span>
          </div>
        </div>

        <div className="flex flex-row items-center p-2">
          <span className="grow px-4">Backlog</span>
          <div className="flex flex-row divide-x">
            {archiveBacklog !== null && (
              <span className="text-sm text-[#aaa] px-4">
                {archiveBacklog} past retention
              </span>
            )}
            {archiveBacklog === null ? (
              <span className="font-bold text-[#5a8d39] px-4">Up to date</span>
            ) : (
              <span className="font-bold text-[#b05656] px-4">Late</span>
            )}
          </div>
        </div>

        <div className="flex flex-row items-center p-2">
          <span className="grow px-4">Failed jobs</span>
          <div className="flex flex-row divide-x">
            <span className="text-sm text-[#aaa] px-4">
              {archiveSnapshotsFailedCount}
            </span>
            {archiveSnapshotsFailedCount > 0 && (
              <span className="font-bold text-[#b05656] px-4">Failing</span>
            )}
          </div>
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
