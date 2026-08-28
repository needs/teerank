import { Metadata } from 'next';
import { notFound } from 'next/navigation';
import { z } from 'zod';
import { format } from 'date-fns';
import { paramsSchema } from '../../schema';
import { encodeString } from '../../../../../utils/encoding';
import { listRecentRecords } from '../../../../../utils/ddnetMap';
import { RecordList } from '../../../../../components/RecordList';
import { DDNetAttribution } from '../../../../../components/DDNetAttribution';

export async function generateMetadata({
  params,
}: {
  params: z.infer<typeof paramsSchema>;
}): Promise<Metadata> {
  const { gameTypeName } = paramsSchema.parse(params);

  return {
    title: `Records - ${gameTypeName}`,
    description: `Latest map records in ${gameTypeName}`,
    alternates: {
      canonical: `https://teerank.io/gametype/${encodeString(gameTypeName)}/records`,
    },
  };
}

export default async function Index({
  params,
}: {
  params: { [key: string]: string };
}) {
  const { gameTypeName } = paramsSchema.parse(params);

  if (gameTypeName !== 'DDraceNetwork') {
    return notFound();
  }

  const records = await listRecentRecords();

  return (
    <>
      <RecordList
        records={records.map((record) => ({
          map: record.map,
          players: record.players,
          time: record.time,
          delta: record.delta,
          dateLabel: format(record.date, 'MMM d, yyyy'),
        }))}
        showMap
        showDelta
      />

      <DDNetAttribution />
    </>
  );
}
