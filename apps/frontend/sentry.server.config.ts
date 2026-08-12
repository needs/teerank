// This file configures the initialization of Sentry on the server.
// The config you add here will be used whenever the server handles a request.
// https://docs.sentry.io/platforms/javascript/guides/nextjs/

import * as Sentry from "@sentry/nextjs";
import prisma from "./utils/prisma";
import { ignoreTransactions, tracesSampleRate, tracesSampler } from "./utils/sentrySampling";

if (process.env.SENTRY_DSN) {
  Sentry.init({
    dsn: process.env.SENTRY_DSN,

    tracesSampleRate,
    tracesSampler,
    ignoreTransactions,

    // Setting this option to true will print useful information to the console while you're setting up Sentry.
    debug: false,

    // uncomment the line below to enable Spotlight (https://spotlightjs.com)
    // spotlight: process.env.NODE_ENV === 'development',

    integrations: [
      new Sentry.Integrations.Prisma({
        client: prisma,
      })
    ]
  });
}
