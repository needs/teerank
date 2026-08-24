import { NextRequest, NextResponse } from 'next/server';
import { CACHE_SECONDS, getDailyPlayers } from '../../../utils/dailyPlayers';

export async function GET(request: NextRequest) {
  return NextResponse.json(
    await getDailyPlayers(request.nextUrl.searchParams.get('range') ?? '90d'),
    {
      headers: {
        'Cache-Control': `public, max-age=${CACHE_SECONDS}, stale-while-revalidate=86400`,
      },
    }
  );
}
