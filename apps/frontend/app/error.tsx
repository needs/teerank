'use client';

import * as Sentry from '@sentry/nextjs';
import Link from 'next/link';
import { useEffect } from 'react';

export default function Error({
  error,
  reset,
}: {
  error: Error & { digest?: string };
  reset: () => void;
}) {
  useEffect(() => {
    // Next.js sets `digest` on errors that originated on the server, and the
    // Sentry server SDK has already reported those. Only report genuine
    // client-side failures here, so a single error is never billed twice.
    if (error.digest === undefined) {
      Sentry.captureException(error);
    }
  }, [error]);

  return (
    <main className="flex flex-col gap-8 py-12">
      <header className="flex flex-col gap-2 px-4 md:px-20">
        <h1 className="text-2xl font-bold">Something went wrong</h1>
        <p>
          This page failed to load. It is usually temporary — trying again
          often works.
        </p>
      </header>

      <div className="flex flex-row gap-4 px-4 md:px-20">
        <button
          onClick={reset}
          className="rounded-md bg-[#e7e5be] px-4 py-2 hover:underline"
        >
          Try again
        </button>
        <Link href="/" className="px-4 py-2 hover:underline">
          Back to home
        </Link>
      </div>

      {error.digest !== undefined && (
        <p className="px-4 md:px-20 text-sm">
          Reference: <code>{error.digest}</code>
        </p>
      )}
    </main>
  );
}
