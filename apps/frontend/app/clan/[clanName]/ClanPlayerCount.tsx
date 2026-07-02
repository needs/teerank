'use client';

import { useSearchParams } from 'next/navigation';

export function ClanPlayerCount({
  activeCount,
  totalCount,
}: {
  activeCount: number;
  totalCount: number;
}) {
  const searchParams = useSearchParams();
  const showPast = searchParams?.has('past') ?? false;

  return (
    <span className="pr-4">
      {activeCount} players
      {showPast && <span className="text-gray-400 ml-1">({totalCount})</span>}
    </span>
  );
}
