import {
  mapCountOldestDate,
  nextMapToCount,
  getQueueUpdatePlayTime,
  lastCompletedJobDate,
  getQueuePollMasterServer,
  getQueuePollGameServer,
  getQueueRankPlayer,
  getQueueGameTypeCount,
} from '@teerank/teerank';
import prisma from '../../utils/prisma';
import { formatDistanceToNow, subMinutes } from 'date-fns';

export const metadata = {
  title: 'Status - Teerank',
  description: 'Teerank status and statistics.',
};

export const dynamic = 'force-dynamic';

export default async function Index() {
  const lastPollMasterServerDate = await lastCompletedJobDate(
    getQueuePollMasterServer()
  );
  const lastPollGameServerDate = await lastCompletedJobDate(
    getQueuePollGameServer()
  );
  const lastPlayTimedSnapshotDate = await lastCompletedJobDate(
    getQueueUpdatePlayTime()
  );
  const lastRankedSnapshotDate = await lastCompletedJobDate(
    getQueueRankPlayer()
  );
  const lastGameTypeCountDate = await lastCompletedJobDate(
    getQueueGameTypeCount()
  );

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

  const nextMapWithOutdatedCounts = await nextMapToCount(prisma);

  const sections = [
    {
      title: 'Polling Master Servers',
      date: lastPollMasterServerDate,
    },
    {
      title: 'Polling Game Servers',
      date: lastPollGameServerDate,
    },
    {
      title: 'Ranking',
      date: lastRankedSnapshotDate,
    },
    {
      title: 'Playtiming',
      date: lastPlayTimedSnapshotDate,
    },
    {
      title: 'Game type count',
      date: lastGameTypeCountDate,
    },
  ];

  return (
    <main className="py-12 px-4 md:px-12 xl:px-20 text-[#666] flex flex-col gap-4">
      <h1 className="text-2xl font-bold clear-both">Teerank</h1>
      <div className="flex flex-col divide-y">
        {sections.map((section) => {
          const isOk =
            section.date !== null && section.date > subMinutes(new Date(), 10);

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

        <div className="flex flex-row items-center p-2">
          <span className="grow px-4">Map count latency</span>
          <div className="flex flex-row divide-x">
            <span className="text-sm text-[#aaa] px-4">
              {formatDistanceToNow(
                nextMapWithOutdatedCounts === null
                  ? mapCountOldestDate()
                  : nextMapWithOutdatedCounts.countedAt
              )}
            </span>
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
