import { Metadata } from 'next';
import { Fragment } from 'react';
import { notFound } from 'next/navigation';
import { z } from 'zod';
import { format } from 'date-fns';
import { localizeUtcDay } from '@teerank/teerank/date';
import prisma from '../../../../../../utils/prisma';
import { paramsSchema } from '../schema';
import { searchParamPageSchema } from '../../../../../../utils/page';
import { sharedHiddenParam } from '../../../../../../utils/shared';
import { encodeString } from '../../../../../../utils/encoding';
import { formatFinishTime, formatTimeDelta } from '../../../../../../utils/format';
import {
  getCompareSplits,
  getDdnetMap,
  getRecordEvents,
  listSoloRecords,
  listTeamRecords,
} from '../../../../../../utils/ddnetMap';
import { StepLineChart } from '../../../../../../components/Chart';
import { RecordList, RecordRanks } from '../../../../../../components/RecordList';
import { DDNetAttribution } from '../../../../../../components/DDNetAttribution';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { gameTypeName, mapName } = paramsSchema.parse(params);

  return {
    title: `${mapName} records - ${gameTypeName}`,
    description: `Record times for ${mapName} in ${gameTypeName}`,
    alternates: {
      canonical: `https://teerank.io/gametype/${encodeString(gameTypeName)}/map/${encodeString(mapName)}/records`,
    },
  };
}

function Checkpoints({
  wrSplits,
  medianSplits,
  compareName,
  compareSplits,
}: {
  wrSplits: number[];
  medianSplits: number[];
  compareName?: string;
  compareSplits: number[] | null;
}) {
  const showCompare = compareName !== undefined && compareSplits !== null;

  const rows = wrSplits.flatMap((wrSplit, index) =>
    wrSplit > 0
      ? [
          {
            checkpoint: index + 1,
            wrSplit,
            medianSplit: medianSplits[index] ?? 0,
            compareSplit: compareSplits?.[index] ?? 0,
          },
        ]
      : []
  );

  const losses = rows.map((row) =>
    row.medianSplit > 0 ? row.medianSplit - row.wrSplit : null
  );
  const maxLoss = Math.max(
    ...losses.filter((loss): loss is number => loss !== null),
    0
  );

  const columnCount = showCompare ? 6 : 4;

  return (
    <details className="px-4 lg:px-8 xl:px-16">
      <summary className="cursor-pointer py-1 text-xl font-bold text-[#970]">
        Checkpoints
      </summary>
      <div
        className="grid gap-x-8 gap-y-2 pt-2"
        style={{
          gridTemplateColumns: `repeat(${columnCount}, max-content)`,
        }}
      >
        <span className="font-bold text-[#970]">CP</span>
        <span className="text-right font-bold text-[#970]">Record split</span>
        <span className="text-right font-bold text-[#970]">Median split</span>
        <span className="text-right font-bold text-[#970]">Time lost</span>
        {showCompare && (
          <span className="truncate text-right font-bold text-[#970]">
            {compareName}
          </span>
        )}
        {showCompare && (
          <span className="text-right font-bold text-[#970]">Δ vs record</span>
        )}

        <span
          className="border-b"
          style={{
            gridColumn: `span ${columnCount} / span ${columnCount}`,
          }}
        />

        {rows.map((row, index) => (
          <Fragment key={row.checkpoint}>
            <span className="text-right">{row.checkpoint}</span>
            <span className="text-right">{formatFinishTime(row.wrSplit)}</span>
            <span className="text-right">
              {row.medianSplit > 0 ? formatFinishTime(row.medianSplit) : ''}
            </span>
            <span
              className={
                losses[index] !== null && losses[index] === maxLoss
                  ? 'text-right text-[#970]'
                  : 'text-right'
              }
            >
              {losses[index] === null ? '' : formatTimeDelta(losses[index] as number)}
            </span>
            {showCompare && (
              <span className="text-right">
                {row.compareSplit > 0 ? formatFinishTime(row.compareSplit) : ''}
              </span>
            )}
            {showCompare && (
              <span className="text-right">
                {row.compareSplit > 0
                  ? formatTimeDelta(row.compareSplit - row.wrSplit)
                  : ''}
              </span>
            )}
          </Fragment>
        ))}
      </div>
    </details>
  );
}

export default async function Index({
  params,
  searchParams,
}: {
  params: { [key: string]: string };
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const { gameTypeName, mapName } = paramsSchema.parse(params);
  const { page } = searchParamPageSchema.parse(searchParams);

  const ranks: RecordRanks =
    searchParams['ranks'] === 'team'
      ? 'team'
      : searchParams['ranks'] === 'duo'
        ? 'duo'
        : 'solo';
  const compareName =
    typeof searchParams['compare'] === 'string'
      ? searchParams['compare']
      : undefined;

  const map = await prisma.map.findUnique({
    select: {
      id: true,
    },
    where: {
      name_gameTypeName: {
        name: mapName,
        gameTypeName: gameTypeName,
      },
    },
  });

  if (map === null) {
    return notFound();
  }

  const ddnetMap = await getDdnetMap(map.id);

  if (ddnetMap === null) {
    return notFound();
  }

  const hideShared = ranks === 'solo' && sharedHiddenParam(searchParams);

  const [events, list, compareSplits] = await Promise.all([
    getRecordEvents(map.id),
    ranks === 'solo'
      ? listSoloRecords({ mapId: map.id, page, hideShared })
      : listTeamRecords({
          mapId: map.id,
          page,
          size: ranks === 'duo' ? 2 : undefined,
        }),
    compareName === undefined || ddnetMap.wrSplits.length === 0
      ? null
      : getCompareSplits(map.id, compareName),
  ]);

  const pathname = `/gametype/${encodeString(gameTypeName)}/map/${encodeString(mapName)}/records`;

  return (
    <>
      {events.length >= 2 && (
        <section className="px-4 lg:px-8 xl:px-16">
          <StepLineChart
            points={events.map((event) => ({
              date: event.date,
              value: event.time,
              label: `${formatFinishTime(event.time)} by ${event.names.join(', ')} — ${format(event.date, 'MMM yyyy')}`,
            }))}
            endDate={new Date()}
          />
        </section>
      )}

      <RecordList
        records={list.records.map((record) => ({
          rank: record.rank,
          players: record.players,
          time: record.time,
          dateLabel:
            ranks === 'solo'
              ? format(localizeUtcDay(record.date), 'MMM d, yyyy')
              : format(record.date, 'MMM d, yyyy'),
        }))}
        recordCount={list.count}
        ranks={ranks}
        pathname={pathname}
        showRank
        showSharedToggle={ranks === 'solo'}
      />

      {ddnetMap.wrSplits.length > 0 && (
        <Checkpoints
          wrSplits={ddnetMap.wrSplits}
          medianSplits={ddnetMap.medianSplits}
          compareName={compareName}
          compareSplits={compareSplits}
        />
      )}

      <DDNetAttribution />
    </>
  );
}
