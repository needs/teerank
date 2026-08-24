import { Duration, intervalToDuration } from 'date-fns';
import Link from 'next/link';
import { twMerge } from 'tailwind-merge';
import { formatDurationShort } from '../utils/format';
import { encodeIp, encodeString } from '../utils/encoding';

export function durationClassName(duration: Duration) {
  if ((duration.years ?? 0) > 0 || (duration.months ?? 0) > 0) {
    return 'text-[#c6c6c6]';
  } else if ((duration.weeks ?? 0) > 0 || (duration.days ?? 0) > 0) {
    return 'text-[#a7a7a7]';
  } else if ((duration.hours ?? 0) > 0) {
    return 'text-[#7fa764]';
  } else {
    return 'text-[#56a721]';
  }
}

export function LastSeen({
  lastSeenAt,
  gameServers,
  playerName,
  className: propClassName,
}: {
  lastSeenAt: Date;
  gameServers: {
    ip: string;
    port: number;
  }[];
  playerName?: string;
  className?: string;
}) {
  if (gameServers.length > 0) {
    const pathname =
      gameServers.length > 1 && playerName !== undefined
        ? `/player/${encodeString(playerName)}/activity`
        : `/server/${encodeIp(gameServers[0].ip)}/${gameServers[0].port}`;

    return (
      <Link
        prefetch={false}
        href={{ pathname }}
        className={twMerge("text-[#43a700] font-bold hover:underline", propClassName)}
      >
        Online
      </Link>
    );
  } else {
    const duration = intervalToDuration({ start: lastSeenAt, end: new Date() });
    const className = twMerge(
      'truncate',
      durationClassName(duration),
      propClassName
    );

    if (playerName !== undefined) {
      return (
        <Link
          prefetch={false}
          href={{ pathname: `/player/${encodeString(playerName)}/activity` }}
          className={twMerge(className, 'hover:underline')}
        >
          {formatDurationShort(duration)}
        </Link>
      );
    }

    return <span className={className}>{formatDurationShort(duration)}</span>;
  }
}
