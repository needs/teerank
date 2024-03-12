import { intervalToDuration } from 'date-fns';
import Link from 'next/link';
import { twMerge } from 'tailwind-merge';
import { formatDurationShort } from '../utils/format';
import { encodeIp, encodeString } from '../utils/encoding';

export function LastSeen({
  lastSnapshot,
  lastSeen,
}: {
  lastSnapshot: {
    ip: string;
    port: number;
  } | null;

  lastSeen: Date;
}) {
  if (lastSnapshot !== null) {
    return (
      <Link
        prefetch={false}
        href={{
          pathname: `/server/${encodeIp(lastSnapshot.ip)}/${lastSnapshot.port}`,
        }}
        className="text-[#43a700] font-bold hover:underline"
      >
        Online
      </Link>
    );
  } else {
    const { years, months, weeks, days, hours } = intervalToDuration({
      start: lastSeen,
      end: new Date(),
    });

    let className;

    if ((years ?? 0) > 0 || (months ?? 0) > 0) {
      className = 'text-[#c6c6c6]';
    } else if ((weeks ?? 0) > 0 || (days ?? 0) > 0) {
      className = 'text-[#a7a7a7]';
    } else if ((hours ?? 0) > 0) {
      className = 'text-[#7fa764]';
    } else {
      className = 'text-[#56a721]';
    }

    return (
      <span className={twMerge('truncate', className)}>
        {formatDurationShort(
          intervalToDuration({ start: lastSeen, end: new Date() })
        )}
      </span>
    );
  }
}
