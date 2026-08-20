import { NextRequest, NextResponse } from 'next/server';
import { getDailyPlayers } from '../../../utils/dailyPlayers';

export async function GET(request: NextRequest) {
  return NextResponse.json(
    await getDailyPlayers(request.nextUrl.searchParams.get('range') ?? '90d')
  );
}
