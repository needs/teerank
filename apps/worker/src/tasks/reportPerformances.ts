const jobPerformances = new Map<string, {
  numberOfBusyRun: number;
  totalBusyDuration: number;
  maxBusyDuration: number;

  numberOfIdleRun: number;
  totalIdleDuration: number;
  maxIdleDuration: number;
}>();

export async function monitorJobPerformance(jobName: string, job: () => Promise<boolean>) {
  if (job === reportPerformances) {
    return await job();
  }

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
  return isBusy;
}

function formatDuration(duration: number) {
  if (isNaN(duration)) {
    return 0;
  }

  return Number(duration.toFixed(3));
}

export async function reportPerformances() {
  const performanceSummary = [...jobPerformances.entries()].sort((a, b) => {
    return Math.max(b[1].maxBusyDuration, b[1].maxIdleDuration) - Math.max(a[1].maxBusyDuration, a[1].maxIdleDuration);
  }).map(([jobName, performanceData]) => {
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

  console.table(performanceSummary);
  jobPerformances.clear();

  return true;
}
