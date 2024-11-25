import { intervalToDuration } from 'date-fns';
import Link from 'next/link';
import { twMerge } from 'tailwind-merge';
import { formatDurationShort } from '../utils/format';
import { encodeIp } from '../utils/encoding';

export function LastSeen({
  lastSeenAt,
  gameServers,
}: {
  lastSeenAt: Date;
  gameServers: {
    ip: string;
    port: number;
  }[];
}) {
  if (gameServers.length > 0) {
    return (
      <Link
        prefetch={false}
        href={{
          pathname: `/server/${encodeIp(gameServers[0].ip)}/${gameServers[0].port}`,
        }}
        className="text-[#43a700] font-bold hover:underline"
      >
        Online
      </Link>
    );
  } else {
    const { years, months, weeks, days, hours } = intervalToDuration({
      start: lastSeenAt,
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
          intervalToDuration({ start: lastSeenAt, end: new Date() })
        )}
      </span>
    );
  }
}
