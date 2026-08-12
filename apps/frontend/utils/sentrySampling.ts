import { isbot } from 'isbot';
import { z } from 'zod';

const DEFAULT_TRACES_SAMPLE_RATE = 0.01;

export const tracesSampleRate = z
  .string()
  .trim()
  .min(1)
  .pipe(z.coerce.number().min(0).max(1))
  .catch(DEFAULT_TRACES_SAMPLE_RATE)
  .parse(process.env.SENTRY_TRACES_SAMPLE_RATE);

export const ignoreTransactions = [
  '/monitoring',
  '/_next/',
  '/favicon.ico',
  '/robots.txt',
  '/sitemap',
];

type SamplingContext = {
  request?: { headers?: Record<string, string> };
};

export function tracesSampler({ request }: SamplingContext) {
  return isbot(request?.headers?.['user-agent']) ? 0 : tracesSampleRate;
}
