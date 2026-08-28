import { Metadata } from 'next';
import { notFound } from 'next/navigation';
import { Fragment } from 'react';
import { z } from 'zod';
import { format } from 'date-fns';
import { localizeUtcDay } from '@teerank/teerank/date';
import { paramsSchema } from './schema';
import { encodeString } from '../../../utils/encoding';
import { formatFinishTime, formatInteger } from '../../../utils/format';
import { getMapperMaps } from '../../../utils/ddnetMap';
import { List, ListCell } from '../../../components/List';
import { DDNetAttribution } from '../../../components/DDNetAttribution';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { mapperName } = paramsSchema.parse(params);

  return {
    title: `Mapper ${mapperName}`,
    description: `Maps made by ${mapperName}`,
    alternates: {
      canonical: `https://teerank.io/mapper/${encodeString(mapperName)}`,
    },
  };
}

export default async function Index({
  params,
}: {
  params: { [key: string]: string };
}) {
  const { mapperName } = paramsSchema.parse(params);

  const maps = await getMapperMaps(mapperName);

  if (maps === null) {
    return notFound();
  }

  const totalFinishes = maps.reduce((sum, map) => sum + map.finishCount, 0);
  const releaseYears = maps.flatMap((map) =>
    map.releasedAt === null ? [] : [localizeUtcDay(map.releasedAt).getFullYear()]
  );

  const minYear = Math.min(...releaseYears);
  const maxYear = Math.max(...releaseYears);
  const releasedLabel =
    releaseYears.length === 0
      ? ''
      : minYear === maxYear
        ? ` · released ${minYear}`
        : ` · released ${minYear}–${maxYear}`;

  return (
    <main className="flex flex-col gap-4 py-8">
      <header className="flex flex-col gap-2 px-8 xl:px-20">
        <h1 className="text-2xl font-bold">{mapperName}</h1>
        <span className="text-sm text-[#999]">
          {formatInteger(maps.length)} {maps.length === 1 ? 'map' : 'maps'} ·{' '}
          {formatInteger(totalFinishes)} total finishes
          {releasedLabel}
        </span>
        <DDNetAttribution label="Map data from" />
      </header>

      <List
        columns={[
          { title: 'Map', expand: true },
          { title: 'Category', expand: false },
          { title: 'Released', expand: false },
          { title: 'Points', expand: false },
          { title: 'Finishes', expand: false },
          { title: 'Median time', expand: false },
        ]}
      >
        {maps.map((map) => (
          <Fragment key={map.name}>
            <ListCell
              label={map.name}
              href={{
                pathname: `/gametype/${encodeString(map.gameTypeName)}/map/${encodeString(map.name)}`,
              }}
            />
            <ListCell label={map.category} />
            <ListCell
              alignRight
              label={
                map.releasedAt === null
                  ? ''
                  : format(localizeUtcDay(map.releasedAt), 'MMM yyyy')
              }
            />
            <ListCell alignRight label={formatInteger(map.points)} />
            <ListCell alignRight label={formatInteger(map.finishCount)} />
            <ListCell
              alignRight
              label={
                map.medianTime === null ? '' : formatFinishTime(map.medianTime)
              }
            />
          </Fragment>
        ))}
      </List>
    </main>
  );
}
