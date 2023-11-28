import { addDefaultMasterServers } from './addDefaultMasterServers';
import { pollGameServers } from './pollGameServers';
import { pollMasterServers } from './pollMasterServers';
import { rankPlayers } from './rankPlayers';
import { removeBadIps } from './removeBadIps';
import { updateGameTypesRankMethod } from './updateGameTypesRankMethod';
import { prisma } from "./prisma";
import * as Sentry from "@sentry/node";
import { ProfilingIntegration } from "@sentry/profiling-node";
import { playTimePlayers, resetPlayTime } from './playTimePlayers';

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
  await removeBadIps();

  await addDefaultMasterServers();
  await updateGameTypesRankMethod();

  let isRunning = false;

  const callback = async () => {
    if (!isRunning) {
      isRunning = true;
      await pollMasterServers();
      await pollGameServers();
      await rankPlayers();
      await playTimePlayers();
      isRunning = false;
    } else {
      console.warn('Skipping poll because it is already running');
    }
  };

  callback();
  setInterval(callback, 1000 * 60 * 5);
}

main();
