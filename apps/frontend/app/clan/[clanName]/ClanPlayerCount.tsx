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
  const past = searchParams?.get('past');
  const showPast = past === 'true' || past === '1';

  return (
    <span className="pr-4">
      {activeCount} players
      {showPast && <span className="text-gray-400 ml-1">({totalCount})</span>}
    </span>
  );
}
