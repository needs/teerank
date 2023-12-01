import prisma from '../../utils/prisma';
import { TaskRunStatus } from '@prisma/client';
import { formatDistanceToNow, subDays } from 'date-fns';
import { formatCamelCase } from '../../utils/format';

export const metadata = {
  title: 'Status',
  description: 'Teerank status',
};

export default async function Index() {
  const tasks = await prisma.task.findMany({
    select: {
      name: true,
      _count: {
        select: {
          runs: {
            where: {
              status: TaskRunStatus.FAILED,
              startedAt: {
                gt: subDays(new Date(), 1),
              },
            },
          },
        },
      },
      runs: {
        orderBy: {
          startedAt: 'desc',
        },
        take: 1,
      },
    },
    where: {
      completedAt: null,
    },
    orderBy: {
      createdAt: 'asc',
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
    orderBy: {
      createdAt: 'asc',
    },
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
          }
        }
      },
    }),
    prisma.gameServerSnapshot.count({
      where: {
        map: {
          gameType: {
            rankMethod: {
              not: null,
            },
          }
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

  return (
    <main className="py-12 px-20 text-[#666] flex flex-col gap-4">
      <h1 className="text-2xl font-bold clear-both">Teerank</h1>
      <div className="flex flex-col divide-y">
        {tasks.map((task) => {
          const lastRun = task.runs[0];

          const isRunning = lastRun.status === null;
          const isFailed = lastRun.status === TaskRunStatus.FAILED;
          const isOk =
            lastRun.status !== TaskRunStatus.FAILED && lastRun.status !== null;

          return (
            <div key={task.name} className="flex flex-row items-center p-2">
              <span className="grow px-4">{formatCamelCase(task.name)}</span>
              <div className="flex flex-row divide-x">
                {task._count.runs > 0 && (
                  <span className="text-sm text-[#d1a4a4] px-4">
                    {task._count.runs} failures
                  </span>
                )}
                <span className="text-sm text-[#aaa] px-4">
                  {formatDistanceToNow(new Date(lastRun.startedAt), {
                    addSuffix: true,
                  })}
                </span>
                {isFailed && (
                  <span className="font-bold text-[#b05656] px-4">Error</span>
                )}
                {isOk && (
                  <span className="font-bold text-[#5a8d39] px-4">OK</span>
                )}
                {isRunning && (
                  <span className="font-bold text-[#5499b0] px-4">Running</span>
                )}
              </div>
            </div>
          );
        })}
        <div className="flex flex-row items-center p-4 gap-4 text-sm text-[#aaa] text-center">
          <span className='grow'>
            {`${Intl.NumberFormat('en-US', {
              maximumFractionDigits: 0,
            }).format(snapshotsCount)} snapshots`}
          </span>
          <span className='grow'>
            {`${Intl.NumberFormat('en-US', {
              maximumFractionDigits: 1,
            }).format(
              rankedSnapshotsCount / rankableSnapshotsCount * 100.0
            )}% ranked`}
          </span>
          <span className='grow'>
            {`${Intl.NumberFormat('en-US', {
              maximumFractionDigits: 1,
            }).format(
              playTimedSnapshotsCount / snapshotsCount * 100.0
            )}% play timed`}
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
