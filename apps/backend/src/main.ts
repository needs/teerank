import { pollGameServers } from './tasks/pollGameServers';
import { pollMasterServers } from './tasks/pollMasterServers';
import { rankPlayers } from './tasks/rankPlayers';
import { prisma } from "./prisma";
import * as Sentry from "@sentry/node";
import { ProfilingIntegration } from "@sentry/profiling-node";
import { playTimePlayers } from './tasks/playTimePlayers';
import { runTasks } from './task';
import { removeBadIps } from './tasks/removeBadIps';
import { addDefaultGameTypes } from './tasks/addDefaultGameTypes';
import { addDefaultMasterServers } from './tasks/addDefaultMasterServers';

if (process.env.NODE_ENV === 'production') {
  Sentry.init({
    dsn: 'https://ed10e9ae17c49680ba4ac68048b7a615@o4506165654192128.ingest.sentry.io/4506165660286976',

    integrations: [
      new ProfilingIntegration(),
      new Sentry.Integrations.Prisma({ client: prisma }),
      new Sentry.Integrations.Console(),
      new Sentry.Integrations.Postgres(),
    ],
    // Performance Monitoring
    tracesSampleRate: 1.0,
    // Set sampling rate for profiling - this is relative to tracesSampleRate
    profilesSampleRate: 1.0,
  });
}

async function main() {
  const tasks = {
    removeBadIps,
    addDefaultGameTypes,
    addDefaultMasterServers,

    pollMasterServers,
    pollGameServers,
    rankPlayers,
    playTimePlayers,
  };

  let isRunning = false;

  const callback = async () => {
    if (!isRunning) {
      isRunning = true;
      await runTasks(tasks);
      isRunning = false;
    } else {
      console.warn('Skipping a new run because one is already in progress');
    }
  };

  callback();
  setInterval(callback, 1000 * 60 * 5);
}

main();
