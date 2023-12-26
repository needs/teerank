import { addDefaultGameTypes } from "./tasks/addDefaultGameTypes";
import { addDefaultMasterServers } from "./tasks/addDefaultMasterServers";
import { cleanStuckJobs } from "./tasks/cleanStuckJobs";
import { pollGameServers } from "./tasks/pollGameServers";
import { pollMasterServers } from "./tasks/pollMasterServers";
import { rankPlayers } from "./tasks/rankPlayers";
import { updatePlayTimes } from "./tasks/updatePlayTime";
import { cancellableWait } from "./utils";

let stopGracefully = false;
const cancellableWaits = new Set<() => void>();

function cancelGracefully() {
  cancellableWaits.forEach(cancel => cancel());
}

process.on('SIGINT', () => {
  console.log('(SIGINT) Stopping gracefully...');
  stopGracefully = true;
  cancelGracefully();
});

const jobPerformances = new Map<string, {
  numberOfBusyRun: number;
  totalBusyDuration: number;
  maxBusyDuration: number;

  numberOfIdleRun: number;
  totalIdleDuration: number;
  maxIdleDuration: number;
}>();

function formatDuration(duration: number) {
  if (isNaN(duration)) {
    return 0;
  }

  return Number(duration.toFixed(3));
}

async function printPerformanceSummary() {
  const performanceSummary = [...jobPerformances.entries()].map(([jobName, performanceData]) => {
    return {
      jobName,

      busyRun: performanceData.numberOfBusyRun,
      busyAvg: formatDuration(performanceData.totalBusyDuration / performanceData.numberOfBusyRun),
      busyMax: formatDuration(performanceData.maxBusyDuration),

      idleRun: performanceData.numberOfIdleRun,
      idleAvg: formatDuration(performanceData.totalIdleDuration / performanceData.numberOfIdleRun ?? undefined),
      idleMax: formatDuration(performanceData.maxIdleDuration),
    };
  });

  if (performanceSummary.length === 0) {
    return false;
  }

  console.table(performanceSummary.filter(({ jobName }) => jobName !== 'printPerformanceSummary'));
  jobPerformances.clear();

  return true;
}

async function runJob(job: () => Promise<boolean>, jobName: string, delayOnBusy: number, delayOnIdle: number) {
  console.log(`Starting ${jobName}`);

  while (!stopGracefully) {
    performance.mark(`${jobName}-start`);
    const isBusy = await job();
    performance.mark(`${jobName}-end`);
    const measure = performance.measure(jobName, `${jobName}-start`, `${jobName}-end`);

    const performanceData = jobPerformances.get(jobName) ?? {
      numberOfBusyRun: 0,
      totalBusyDuration: 0,
      maxBusyDuration: 0,

      numberOfIdleRun: 0,
      totalIdleDuration: 0,
      maxIdleDuration: 0,
    };

    if (isBusy) {
      performanceData.numberOfBusyRun++;
      performanceData.totalBusyDuration += measure.duration;
      performanceData.maxBusyDuration = Math.max(performanceData.maxBusyDuration, measure.duration);
    } else {
      performanceData.numberOfIdleRun++;
      performanceData.totalIdleDuration += measure.duration;
      performanceData.maxIdleDuration = Math.max(performanceData.maxIdleDuration, measure.duration);
    }

    jobPerformances.set(jobName, performanceData);

    const { wait, cancel } = cancellableWait(isBusy ? delayOnBusy : delayOnIdle);

    cancellableWaits.add(cancel);
    if (!stopGracefully) {
      await wait;
    }
    cancellableWaits.delete(cancel);
  }

  console.log(`Stopped ${jobName} gracefully`);
}

async function main() {
  await addDefaultGameTypes();
  await addDefaultMasterServers();

  await Promise.all([
    runJob(cleanStuckJobs, 'cleanStuckJobs', 5000, 60000),
    runJob(pollMasterServers, 'pollMasterServers', 0, 60000),
    runJob(pollGameServers, 'pollGameServers', 0, 5000),
    runJob(rankPlayers, 'rankPlayers', 0, 5000),
    runJob(updatePlayTimes, 'updatePlayTimes', 0, 5000),
    runJob(printPerformanceSummary, 'printPerformanceSummary', 60000, 60000),
  ]);

  console.log('Stopped gracefully');
}

main()
